/*
** Quentin FILLIAERT, 2026
** light_control.c
** File description:
** File containing the logical light control and mixing API
*/

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <lightplayer/led_pwm.h>
#include <lightplayer/light_control.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef BIT
#define BIT(n) (1U << (n))
#endif

#define LIGHT_CAL_SCALE 1000U
#define LIGHT_LINEAR_MAX UINT16_MAX

static_assert(LIGHT_STATE_PAYLOAD_SIZE == 12, "LIGHT_STATE payload size");

/*
 * 16-bit sRGB-to-linear lookup table for 0..255 app values.
 * The mixer works in linear light so brightness and duplicated-channel
 * decisions do not treat perceptual sRGB bytes as physical PWM duty directly.
 */
static const uint16_t srgb_to_linear[256] = {
    0,     20,    40,    60,    80,    99,    119,   139,   159,   179,   199,   219,   241,
    264,   288,   313,   340,   367,   396,   427,   458,   491,   526,   562,   599,   637,
    677,   718,   761,   805,   851,   898,   947,   997,   1048,  1101,  1156,  1212,  1270,
    1330,  1391,  1453,  1517,  1583,  1651,  1720,  1790,  1863,  1937,  2013,  2090,  2170,
    2250,  2333,  2418,  2504,  2592,  2681,  2773,  2866,  2961,  3058,  3157,  3258,  3360,
    3464,  3570,  3678,  3788,  3900,  4014,  4129,  4247,  4366,  4488,  4611,  4736,  4864,
    4993,  5124,  5257,  5392,  5530,  5669,  5810,  5953,  6099,  6246,  6395,  6547,  6700,
    6856,  7014,  7174,  7335,  7500,  7666,  7834,  8004,  8177,  8352,  8528,  8708,  8889,
    9072,  9258,  9445,  9635,  9828,  10022, 10219, 10417, 10619, 10822, 11028, 11235, 11446,
    11658, 11873, 12090, 12309, 12530, 12754, 12980, 13209, 13440, 13673, 13909, 14146, 14387,
    14629, 14874, 15122, 15371, 15623, 15878, 16135, 16394, 16656, 16920, 17187, 17456, 17727,
    18001, 18277, 18556, 18837, 19121, 19407, 19696, 19987, 20281, 20577, 20876, 21177, 21481,
    21787, 22096, 22407, 22721, 23038, 23357, 23678, 24002, 24329, 24658, 24990, 25325, 25662,
    26001, 26344, 26688, 27036, 27386, 27739, 28094, 28452, 28813, 29176, 29542, 29911, 30282,
    30656, 31033, 31412, 31794, 32179, 32567, 32957, 33350, 33745, 34143, 34544, 34948, 35355,
    35764, 36176, 36591, 37008, 37429, 37852, 38278, 38706, 39138, 39572, 40009, 40449, 40891,
    41337, 41785, 42236, 42690, 43147, 43606, 44069, 44534, 45002, 45473, 45947, 46423, 46903,
    47385, 47871, 48359, 48850, 49344, 49841, 50341, 50844, 51349, 51858, 52369, 52884, 53401,
    53921, 54445, 54971, 55500, 56032, 56567, 57105, 57646, 58190, 58737, 59287, 59840, 60396,
    60955, 61517, 62082, 62650, 63221, 63795, 64372, 64952, 65535};

/**
 * @brief Convert a validated BLE 0/1 byte into a boolean.
 */
static bool command_bool(uint8_t value)
{
    return value != 0;
}

/**
 * @brief Return the Kconfig-selected startup mix policy.
 */
static light_mix_policy_t default_mix_policy(void)
{
#ifdef CONFIG_LIGHT_PLAYER_DEFAULT_MIX_POLICY_MAX_BRIGHTNESS
    return LIGHT_MIX_POLICY_MAX_BRIGHTNESS;
#else
    return LIGHT_MIX_POLICY_COLOR_ACCURATE;
#endif
}

/**
 * @brief Clamp a wider calculation to the internal 16-bit linear range.
 */
static uint16_t clamp_linear(uint32_t value)
{
    return (uint16_t)MIN(value, LIGHT_LINEAR_MAX);
}

/**
 * @brief Apply perceptual brightness to an sRGB component, then convert to linear light.
 *
 * BLE RGB and brightness values are app-facing sRGB/perceptual bytes. Scaling
 * the byte first keeps mixed-color hue stable at low brightness; multiplying
 * two already-linearized values makes secondary channels disappear too early.
 */
static uint16_t apply_brightness(uint8_t component, uint8_t brightness)
{
    uint8_t scaled_component =
        (uint8_t)(((uint32_t)component * brightness + UINT8_MAX / 2U) / UINT8_MAX);

    return srgb_to_linear[scaled_component];
}

/**
 * @brief Convert linear PWM demand to a hardware PWM level using physical gain.
 *
 * Higher channel gain means the firmware assumes the physical output is
 * brighter, so less duty is needed for the same requested light.
 */
static led_pwm_level_t level_to_pwm(uint32_t level, uint32_t gain)
{
    const uint32_t adjusted = MIN((level * LIGHT_CAL_SCALE) / gain, LIGHT_LINEAR_MAX);
    uint32_t pwm = (adjusted * LED_PWM_LEVEL_MAX) / LIGHT_LINEAR_MAX;

    /*
     * Avoid exact 100% duty on Nordic pwm_nrfx.
     *
     * An exact full-scale level creates pulse == period. On nRF54L15 /
     * Zephyr pwm_nrfx, this can leave the channel in a special constant-active state.
     * Updating another channel on the same PWM peripheral afterwards can then
     * fail with:
     *
     *   pwm_nrfx: Incompatible period
     *
     * One count below full scale is visually indistinguishable from full-on
     * for LED dimming.
     */
    if (pwm >= LED_PWM_LEVEL_MAX)
    {
        pwm = LED_PWM_LEVEL_MAX - 1U;
    }

    return (led_pwm_level_t)pwm;
}

/**
 * @brief Add two linear levels with saturation.
 */
static uint16_t add_linear(uint16_t first, uint32_t second)
{
    return clamp_linear((uint32_t)first + second);
}

/**
 * @brief Return value - reference when value is greater, otherwise zero.
 */
static uint16_t subtract_if_greater(uint16_t value, uint16_t reference)
{
    if (value <= reference)
        return 0;
    return value - reference;
}

/**
 * @brief Split a red or blue booster demand across the duplicated channels.
 *
 * The total duplicated capacity is scaled by the configured physical gains and
 * then split proportionally. With both gains at 1000, a pure red/blue request
 * can drive both same-color outputs at full scale in color-accurate mode.
 */
static void split_boosted_level(uint16_t level, uint32_t first_gain, uint32_t second_gain,
                                uint32_t *first, uint32_t *second)
{
    const uint32_t total_gain = first_gain + second_gain;
    const uint32_t boosted_level = (uint32_t)(((uint64_t)level * total_gain) / LIGHT_CAL_SCALE);

    *first = (uint32_t)(((uint64_t)boosted_level * first_gain) / total_gain);
    *second = boosted_level - *first;
}

/**
 * @brief Convert a single-channel strip brightness to one physical PWM level.
 *
 * No luminous color weighting is applied in direct per-strip mode.
 * The brightness byte is app-facing/perceptual and is converted to linear light.
 */
static led_pwm_level_t single_pwm(uint8_t brightness, uint32_t gain)
{
    const uint16_t level = srgb_to_linear[brightness];

    return level_to_pwm(level, gain);
}

/**
 * @brief Convert an RGB strip color component and brightness to one PWM level.
 *
 * The brightness byte is app-facing/perceptual and is converted to linear light.
 * RGB component ratios are preserved across brightness changes. Photopic
 * channel data is kept as reference calibration, but it is not inverted into
 * per-channel PWM here because that clips weak channels and changes hue while
 * dimming.
 */
static led_pwm_level_t rgb_pwm(uint8_t component, uint8_t brightness, uint32_t gain)
{
    const uint16_t level = apply_brightness(component, brightness);

    return level_to_pwm(level, gain);
}

/**
 * @brief Direct per-strip mapping into RGB_R, RGB_G, RGB_B, FlameWarm, SkyBlue.
 */
static void mix_per_strip(const light_state_t *state, led_pwm_level_t levels[LED_PWM_CHANNEL_COUNT])
{
    levels[LED_PWM_CHANNEL_RGB_RED] =
        rgb_pwm(state->rgb_strip.r, state->rgb_strip.brightness, CONFIG_LIGHT_PLAYER_RGB_RED_GAIN);
    levels[LED_PWM_CHANNEL_RGB_GREEN] = rgb_pwm(state->rgb_strip.g, state->rgb_strip.brightness,
                                                CONFIG_LIGHT_PLAYER_RGB_GREEN_GAIN);
    levels[LED_PWM_CHANNEL_RGB_BLUE] = rgb_pwm(state->rgb_strip.b, state->rgb_strip.brightness,
                                               CONFIG_LIGHT_PLAYER_RGB_BLUE_GAIN);
    levels[LED_PWM_CHANNEL_FLAME_WARM] =
        single_pwm(state->flamewarm_brightness, CONFIG_LIGHT_PLAYER_FLAMEWARM_GAIN);
    levels[LED_PWM_CHANNEL_SKY_BLUE] =
        single_pwm(state->skyblue_brightness, CONFIG_LIGHT_PLAYER_SKYBLUE_GAIN);
}

/**
 * @brief Global mode that uses duplicated red/blue channels aggressively.
 *
 * Red requests drive both RGB_R and FlameWarm. Blue requests drive both RGB_B
 * and SkyBlue. Green is available only on RGB_G.
 */
static void mix_global_max_brightness(const light_state_t *state,
                                      led_pwm_level_t levels[LED_PWM_CHANNEL_COUNT])
{
    const uint16_t red = apply_brightness(state->global.r, state->global.brightness);
    const uint16_t green = apply_brightness(state->global.g, state->global.brightness);
    const uint16_t blue = apply_brightness(state->global.b, state->global.brightness);

    levels[LED_PWM_CHANNEL_RGB_RED] = level_to_pwm(red, CONFIG_LIGHT_PLAYER_RGB_RED_GAIN);
    levels[LED_PWM_CHANNEL_RGB_GREEN] = level_to_pwm(green, CONFIG_LIGHT_PLAYER_RGB_GREEN_GAIN);
    levels[LED_PWM_CHANNEL_RGB_BLUE] = level_to_pwm(blue, CONFIG_LIGHT_PLAYER_RGB_BLUE_GAIN);
    levels[LED_PWM_CHANNEL_FLAME_WARM] = level_to_pwm(red, CONFIG_LIGHT_PLAYER_FLAMEWARM_GAIN);
    levels[LED_PWM_CHANNEL_SKY_BLUE] = level_to_pwm(blue, CONFIG_LIGHT_PLAYER_SKYBLUE_GAIN);
}

/**
 * @brief Global mode that preserves mixed colors while adding dominant red/blue capacity.
 *
 * The RGB strip carries the shared color component. FlameWarm is added only for
 * red that exceeds both green and blue. SkyBlue is added only for blue that
 * exceeds both red and green. This keeps white and balanced secondary colors
 * from being shifted by the duplicated single-color strips.
 */
static void mix_global_color_accurate(const light_state_t *state,
                                      led_pwm_level_t levels[LED_PWM_CHANNEL_COUNT])
{
    const uint16_t raw_red = apply_brightness(state->global.r, state->global.brightness);
    const uint16_t raw_green = apply_brightness(state->global.g, state->global.brightness);
    const uint16_t raw_blue = apply_brightness(state->global.b, state->global.brightness);
    const uint16_t red_reference = MAX(raw_green, raw_blue);
    const uint16_t blue_reference = MAX(raw_red, raw_green);
    const uint16_t raw_red_boost = subtract_if_greater(raw_red, red_reference);
    const uint16_t raw_blue_boost = subtract_if_greater(raw_blue, blue_reference);
    const uint16_t red_base = raw_red - raw_red_boost;
    const uint16_t blue_base = raw_blue - raw_blue_boost;
    uint32_t rgb_red_boost;
    uint32_t flamewarm_boost;
    uint32_t rgb_blue_boost;
    uint32_t skyblue_boost;

    split_boosted_level(raw_red_boost, CONFIG_LIGHT_PLAYER_RGB_RED_GAIN,
                        CONFIG_LIGHT_PLAYER_FLAMEWARM_GAIN, &rgb_red_boost, &flamewarm_boost);
    split_boosted_level(raw_blue_boost, CONFIG_LIGHT_PLAYER_RGB_BLUE_GAIN,
                        CONFIG_LIGHT_PLAYER_SKYBLUE_GAIN, &rgb_blue_boost, &skyblue_boost);

    levels[LED_PWM_CHANNEL_RGB_RED] =
        level_to_pwm(add_linear(red_base, rgb_red_boost), CONFIG_LIGHT_PLAYER_RGB_RED_GAIN);
    levels[LED_PWM_CHANNEL_RGB_GREEN] =
        level_to_pwm(raw_green, CONFIG_LIGHT_PLAYER_RGB_GREEN_GAIN);
    levels[LED_PWM_CHANNEL_RGB_BLUE] =
        level_to_pwm(add_linear(blue_base, rgb_blue_boost), CONFIG_LIGHT_PLAYER_RGB_BLUE_GAIN);
    levels[LED_PWM_CHANNEL_FLAME_WARM] =
        level_to_pwm(flamewarm_boost, CONFIG_LIGHT_PLAYER_FLAMEWARM_GAIN);
    levels[LED_PWM_CHANNEL_SKY_BLUE] =
        level_to_pwm(skyblue_boost, CONFIG_LIGHT_PLAYER_SKYBLUE_GAIN);
}

/**
 * @brief Select and run the configured global RGB mixing policy.
 */
static void mix_global(const light_state_t *state, led_pwm_level_t levels[LED_PWM_CHANNEL_COUNT])
{
    if (state->mix_policy == LIGHT_MIX_POLICY_MAX_BRIGHTNESS)
    {
        mix_global_max_brightness(state, levels);
        return;
    }
    mix_global_color_accurate(state, levels);
}

/**
 * @brief Initialize logical light state and apply the initial master-off output.
 */
int light_control_init(light_state_t *state)
{
    if (state == NULL)
        return -EINVAL;
    memset(state, 0, sizeof(*state));
    state->master_on = false;
    state->global_mode = true;
    state->mix_policy = default_mix_policy();
    state->global = (light_rgb_state_t){
        .brightness = UINT8_MAX, .r = UINT8_MAX, .g = UINT8_MAX, .b = UINT8_MAX};
    state->rgb_strip = state->global;
    state->flamewarm_brightness = UINT8_MAX;
    state->skyblue_brightness = UINT8_MAX;
    return light_control_apply(state);
}

/**
 * @brief Render current logical state to the five physical PWM outputs.
 */
int light_control_render(const light_state_t *state, light_output_frame_t *frame)
{
    if (state == NULL || frame == NULL)
        return -EINVAL;
    memset(frame, 0, sizeof(*frame));
    if (!state->master_on)
        return 0;
    if (state->global_mode)
        mix_global(state, frame->levels);
    else
        mix_per_strip(state, frame->levels);
    return 0;
}

/**
 * @brief Apply a pre-rendered physical PWM output frame.
 */
int light_control_apply_frame(const light_output_frame_t *frame)
{
    if (frame == NULL)
        return -EINVAL;
    return led_pwm_set_levels(frame->levels);
}

/**
 * @brief Apply current logical state to the five physical PWM outputs.
 */
int light_control_apply(const light_state_t *state)
{
    int err;
    light_output_frame_t frame;

    err = light_control_render(state, &frame);
    if (err != 0)
        return err;
    return light_control_apply_frame(&frame);
}

/**
 * @brief Set master output state without changing stored colors or brightness.
 */
void light_control_set_master(light_state_t *state, bool on)
{
    state->master_on = on;
}

/**
 * @brief Set global brightness/color from BLE opcode 0x02 without changing master power.
 */
void light_control_set_global(light_state_t *state, uint8_t brightness, uint8_t r, uint8_t g,
                              uint8_t b)
{
    state->global_mode = true;
    state->global = (light_rgb_state_t){.brightness = brightness, .r = r, .g = g, .b = b};
}

/**
 * @brief Set global brightness from BLE opcode 0x03.
 */
void light_control_set_global_brightness(light_state_t *state, uint8_t brightness)
{
    state->global_mode = true;
    state->global.brightness = brightness;
}

/**
 * @brief Set global color from BLE opcode 0x04.
 */
void light_control_set_global_color(light_state_t *state, uint8_t r, uint8_t g, uint8_t b)
{
    state->global_mode = true;
    state->global.r = r;
    state->global.g = g;
    state->global.b = b;
}

/**
 * @brief Set physical RGB strip state from BLE opcode 0x10.
 */
void light_control_set_rgb_strip(light_state_t *state, uint8_t brightness, uint8_t r, uint8_t g,
                                 uint8_t b)
{
    state->global_mode = false;
    state->rgb_strip = (light_rgb_state_t){.brightness = brightness, .r = r, .g = g, .b = b};
}

/**
 * @brief Set FlameWarm brightness from BLE opcode 0x11.
 */
void light_control_set_flamewarm(light_state_t *state, uint8_t brightness)
{
    state->global_mode = false;
    state->flamewarm_brightness = brightness;
}

/**
 * @brief Set SkyBlue brightness from BLE opcode 0x12.
 */
void light_control_set_skyblue(light_state_t *state, uint8_t brightness)
{
    state->global_mode = false;
    state->skyblue_brightness = brightness;
}

/**
 * @brief Set global mixing policy from BLE opcode 0x20.
 */
void light_control_set_mix_policy(light_state_t *state, light_mix_policy_t policy)
{
    state->mix_policy = policy;
}

/**
 * @brief Serialize light_state_t into the BLE LIGHT_STATE byte layout.
 */
int light_control_encode_state(const light_state_t *state,
                               uint8_t payload[LIGHT_STATE_PAYLOAD_SIZE])
{
    uint8_t flags = 0;

    if (state == NULL || payload == NULL)
        return -EINVAL;
    if (state->master_on)
        flags |= BIT(0);
    if (state->global_mode)
        flags |= BIT(1);
    if (state->mix_policy == LIGHT_MIX_POLICY_MAX_BRIGHTNESS)
        flags |= BIT(2);
    payload[0] = LIGHT_STATE_PAYLOAD_VERSION;
    payload[1] = flags;
    payload[2] = state->global.brightness;
    payload[3] = state->global.r;
    payload[4] = state->global.g;
    payload[5] = state->global.b;
    payload[6] = state->rgb_strip.brightness;
    payload[7] = state->rgb_strip.r;
    payload[8] = state->rgb_strip.g;
    payload[9] = state->rgb_strip.b;
    payload[10] = state->flamewarm_brightness;
    payload[11] = state->skyblue_brightness;
    return 0;
}

/**
 * @brief Validate opcode, payload length and enum-like fields of a LIGHT_COMMAND.
 */
int light_control_validate_command(const uint8_t *command, uint16_t len)
{
    if (len == 0)
        return -EMSGSIZE;
    switch (command[0])
    {
    case LIGHT_CMD_SET_MASTER:
        if (len != 2)
            return -EMSGSIZE;
        return command[1] <= 1 ? 0 : -EINVAL;
    case LIGHT_CMD_SET_POLICY:
        if (len != 2)
            return -EMSGSIZE;
        return command[1] <= LIGHT_MIX_POLICY_MAX_BRIGHTNESS ? 0 : -EINVAL;
    case LIGHT_CMD_SET_GLOBAL_BRIGHTNESS:
    case LIGHT_CMD_SET_FLAMEWARM:
    case LIGHT_CMD_SET_SKYBLUE:
        return len == 2 ? 0 : -EMSGSIZE;
    case LIGHT_CMD_SET_GLOBAL_COLOR:
        return len == 4 ? 0 : -EMSGSIZE;
    case LIGHT_CMD_SET_RGB_STRIP:
        return len == 5 ? 0 : -EMSGSIZE;
    case LIGHT_CMD_SET_GLOBAL:
        return len == 5 ? 0 : -EMSGSIZE;
    default:
        return -EINVAL;
    }
}

/**
 * @brief Apply a validated command to the logical light state.
 *
 * This function assumes light_control_validate_command() has already accepted
 * the payload.
 */
void light_control_apply_command(light_state_t *state, const uint8_t *command)
{
    switch (command[0])
    {
    case LIGHT_CMD_SET_MASTER:
        return light_control_set_master(state, command_bool(command[1]));
    case LIGHT_CMD_SET_GLOBAL:
        return light_control_set_global(state, command[1], command[2], command[3], command[4]);
    case LIGHT_CMD_SET_GLOBAL_BRIGHTNESS:
        return light_control_set_global_brightness(state, command[1]);
    case LIGHT_CMD_SET_GLOBAL_COLOR:
        return light_control_set_global_color(state, command[1], command[2], command[3]);
    case LIGHT_CMD_SET_RGB_STRIP:
        return light_control_set_rgb_strip(state, command[1], command[2], command[3], command[4]);
    case LIGHT_CMD_SET_FLAMEWARM:
        return light_control_set_flamewarm(state, command[1]);
    case LIGHT_CMD_SET_SKYBLUE:
        return light_control_set_skyblue(state, command[1]);
    case LIGHT_CMD_SET_POLICY:
        return light_control_set_mix_policy(state, (light_mix_policy_t)command[1]);
    default:
    }
}
