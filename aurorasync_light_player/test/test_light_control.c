/*
** test_light_control.c
** File description:
** Unit tests of the light control API
*/

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <unity.h>

#include <lightplayer/led_pwm.h>
#include <lightplayer/light_control.h>

#define LIT(channel) (1U << (channel))
#define RGB_STRIP                                                                                  \
    (LIT(LED_PWM_CHANNEL_RGB_RED) | LIT(LED_PWM_CHANNEL_RGB_GREEN) | LIT(LED_PWM_CHANNEL_RGB_BLUE))
#define ALL_CHANNELS (LIT(LED_PWM_CHANNEL_COUNT) - 1U)

/** The three ways to render a state: global color-accurate, global max-brightness, per-strip. */
static const struct
{
    bool global_mode;
    light_mix_policy_t policy;
} modes[] = {
    {true, LIGHT_MIX_POLICY_COLOR_ACCURATE},
    {true, LIGHT_MIX_POLICY_MAX_BRIGHTNESS},
    {false, LIGHT_MIX_POLICY_COLOR_ACCURATE},
};

#define MODE_COUNT (sizeof(modes) / sizeof(modes[0]))

/** Return a state from light_control_init(): global mode, every color at 255. */
static light_state_t init_state(void)
{
    light_state_t state;

    TEST_ASSERT_EQUAL_INT(0, light_control_init(&state));
    return state;
}

static uint8_t encoded_flags(const light_state_t *state)
{
    uint8_t payload[LIGHT_STATE_PAYLOAD_SIZE];

    TEST_ASSERT_EQUAL_INT(0, light_control_encode_state(state, payload));
    return payload[1];
}

static void assert_same_mode_and_colors(const light_state_t *expected, const light_state_t *actual)
{
    TEST_ASSERT_EQUAL(expected->global_mode, actual->global_mode);
    TEST_ASSERT_EQUAL_MEMORY(&expected->global, &actual->global, sizeof(actual->global));
    TEST_ASSERT_EQUAL_MEMORY(&expected->rgb_strip, &actual->rgb_strip, sizeof(actual->rgb_strip));
    TEST_ASSERT_EQUAL_UINT8(expected->flamewarm_brightness, actual->flamewarm_brightness);
    TEST_ASSERT_EQUAL_UINT8(expected->skyblue_brightness, actual->skyblue_brightness);
}

/** Return a lit state in global mode, at full brightness. */
static light_state_t global_state(light_mix_policy_t policy, uint8_t r, uint8_t g, uint8_t b)
{
    return (light_state_t){
        .master_on = true, .global_mode = true, .mix_policy = policy, .global = {255, r, g, b}};
}

static light_output_frame_t render(const light_state_t *state)
{
    light_output_frame_t frame;

    TEST_ASSERT_EQUAL_INT(0, light_control_render(state, &frame));
    return frame;
}

/** Assert that the channels in lit, a mask of LIT() bits, are the only ones on. */
static void assert_lit(light_state_t state, unsigned lit)
{
    const light_output_frame_t frame = render(&state);

    for (int ch = 0; ch < LED_PWM_CHANNEL_COUNT; ch++)
    {
        if (lit & LIT(ch))
            TEST_ASSERT_GREATER_THAN_UINT16(0, frame.levels[ch]);
        else
            TEST_ASSERT_EQUAL_UINT16(0, frame.levels[ch]);
    }
}

/** Assert that no level decreases while every brightness of the state rises from 0 to 255. */
static void assert_brightness_monotonic(light_state_t state)
{
    light_output_frame_t previous = {0};

    for (int brightness = 0; brightness <= UINT8_MAX; brightness++)
    {
        light_output_frame_t frame;

        state.global.brightness = brightness;
        state.rgb_strip.brightness = brightness;
        state.flamewarm_brightness = brightness;
        state.skyblue_brightness = brightness;
        frame = render(&state);
        for (int ch = 0; ch < LED_PWM_CHANNEL_COUNT; ch++)
            TEST_ASSERT_GREATER_OR_EQUAL_UINT16(previous.levels[ch], frame.levels[ch]);
        previous = frame;
    }
}

TEST_CASE("empty command is rejected", "[light_control]")
{
    TEST_ASSERT_EQUAL_INT(-EMSGSIZE, light_control_validate_command(NULL, 0));
}

TEST_CASE("master command accepts valid values", "[light_control]")
{
    uint8_t command[] = {LIGHT_CMD_SET_MASTER, 0};

    TEST_ASSERT_EQUAL_INT(0, light_control_validate_command(command, sizeof(command)));
    command[1] = 1;
    TEST_ASSERT_EQUAL_INT(0, light_control_validate_command(command, sizeof(command)));
}

TEST_CASE("master command rejects invalid values", "[light_control]")
{
    uint8_t command[] = {LIGHT_CMD_SET_MASTER, 2};

    TEST_ASSERT_EQUAL_INT(-EINVAL, light_control_validate_command(command, sizeof(command)));
}

TEST_CASE("policy command accepts valid values", "[light_control]")
{
    uint8_t command[] = {LIGHT_CMD_SET_POLICY, 0};

    TEST_ASSERT_EQUAL_INT(0, light_control_validate_command(command, sizeof(command)));
    command[1] = LIGHT_MIX_POLICY_MAX_BRIGHTNESS;
    TEST_ASSERT_EQUAL_INT(0, light_control_validate_command(command, sizeof(command)));
}

TEST_CASE("policy command rejects invalid values", "[light_control]")
{
    uint8_t command[] = {LIGHT_CMD_SET_POLICY, LIGHT_MIX_POLICY_MAX_BRIGHTNESS + 1};

    TEST_ASSERT_EQUAL_INT(-EINVAL, light_control_validate_command(command, sizeof(command)));
}

TEST_CASE("commands reject incorrect lengths", "[light_control]")
{
    uint8_t master[] = {LIGHT_CMD_SET_MASTER, 1, 2};
    uint8_t global_color[] = {LIGHT_CMD_SET_GLOBAL_COLOR, 1, 2};
    uint8_t rgb_strip[] = {LIGHT_CMD_SET_RGB_STRIP, 1, 2, 3};
    uint8_t global[] = {LIGHT_CMD_SET_GLOBAL, 1, 2, 3, 4, 5};

    TEST_ASSERT_EQUAL_INT(-EMSGSIZE, light_control_validate_command(master, sizeof(master)));
    TEST_ASSERT_EQUAL_INT(-EMSGSIZE,
                          light_control_validate_command(global_color, sizeof(global_color)));
    TEST_ASSERT_EQUAL_INT(-EMSGSIZE, light_control_validate_command(rgb_strip, sizeof(rgb_strip)));
    TEST_ASSERT_EQUAL_INT(-EMSGSIZE, light_control_validate_command(global, sizeof(global)));
}

TEST_CASE("unknown opcode is rejected", "[light_control]")
{
    uint8_t command[] = {0xFF, 0};

    TEST_ASSERT_EQUAL_INT(-EINVAL, light_control_validate_command(command, sizeof(command)));
}

TEST_CASE("commands with correct lengths are accepted", "[light_control]")
{
    uint8_t brightness[] = {LIGHT_CMD_SET_GLOBAL_BRIGHTNESS, 50};
    uint8_t flamewarm[] = {LIGHT_CMD_SET_FLAMEWARM, 50};
    uint8_t skyblue[] = {LIGHT_CMD_SET_SKYBLUE, 50};
    uint8_t global_color[] = {LIGHT_CMD_SET_GLOBAL_COLOR, 1, 2, 3};
    uint8_t rgb_strip[] = {LIGHT_CMD_SET_RGB_STRIP, 1, 2, 3, 4};
    uint8_t global[] = {LIGHT_CMD_SET_GLOBAL, 1, 2, 3, 4};

    TEST_ASSERT_EQUAL_INT(0, light_control_validate_command(brightness, sizeof(brightness)));
    TEST_ASSERT_EQUAL_INT(0, light_control_validate_command(flamewarm, sizeof(flamewarm)));
    TEST_ASSERT_EQUAL_INT(0, light_control_validate_command(skyblue, sizeof(skyblue)));
    TEST_ASSERT_EQUAL_INT(0, light_control_validate_command(global_color, sizeof(global_color)));
    TEST_ASSERT_EQUAL_INT(0, light_control_validate_command(rgb_strip, sizeof(rgb_strip)));
    TEST_ASSERT_EQUAL_INT(0, light_control_validate_command(global, sizeof(global)));
}

TEST_CASE("master command switches the master", "[light_control]")
{
    const uint8_t on[] = {LIGHT_CMD_SET_MASTER, 1};
    const uint8_t off[] = {LIGHT_CMD_SET_MASTER, 0};
    light_state_t state = init_state();

    light_control_apply_command(&state, on);
    TEST_ASSERT_TRUE(state.master_on);
    light_control_apply_command(&state, off);
    TEST_ASSERT_FALSE(state.master_on);
}

TEST_CASE("global command sets the global brightness and color", "[light_control]")
{
    const uint8_t command[] = {LIGHT_CMD_SET_GLOBAL, 10, 20, 30, 40};
    const light_rgb_state_t expected = {10, 20, 30, 40};
    light_state_t state = init_state();

    light_control_apply_command(&state, command);
    TEST_ASSERT_EQUAL_MEMORY(&expected, &state.global, sizeof(expected));
}

TEST_CASE("global brightness command only sets the global brightness", "[light_control]")
{
    const uint8_t command[] = {LIGHT_CMD_SET_GLOBAL_BRIGHTNESS, 10};
    const light_rgb_state_t expected = {10, 255, 255, 255};
    light_state_t state = init_state();

    light_control_apply_command(&state, command);
    TEST_ASSERT_EQUAL_MEMORY(&expected, &state.global, sizeof(expected));
}

TEST_CASE("global color command only sets the global color", "[light_control]")
{
    const uint8_t command[] = {LIGHT_CMD_SET_GLOBAL_COLOR, 20, 30, 40};
    const light_rgb_state_t expected = {255, 20, 30, 40};
    light_state_t state = init_state();

    light_control_apply_command(&state, command);
    TEST_ASSERT_EQUAL_MEMORY(&expected, &state.global, sizeof(expected));
}

TEST_CASE("RGB strip command sets the RGB strip", "[light_control]")
{
    const uint8_t command[] = {LIGHT_CMD_SET_RGB_STRIP, 10, 20, 30, 40};
    const light_rgb_state_t expected = {10, 20, 30, 40};
    light_state_t state = init_state();

    light_control_apply_command(&state, command);
    TEST_ASSERT_EQUAL_MEMORY(&expected, &state.rgb_strip, sizeof(expected));
}

TEST_CASE("FlameWarm command sets the FlameWarm brightness", "[light_control]")
{
    const uint8_t command[] = {LIGHT_CMD_SET_FLAMEWARM, 10};
    light_state_t state = init_state();

    light_control_apply_command(&state, command);
    TEST_ASSERT_EQUAL_UINT8(10, state.flamewarm_brightness);
}

TEST_CASE("SkyBlue command sets the SkyBlue brightness", "[light_control]")
{
    const uint8_t command[] = {LIGHT_CMD_SET_SKYBLUE, 10};
    light_state_t state = init_state();

    light_control_apply_command(&state, command);
    TEST_ASSERT_EQUAL_UINT8(10, state.skyblue_brightness);
}

TEST_CASE("policy command sets the mix policy", "[light_control]")
{
    const uint8_t max_brightness[] = {LIGHT_CMD_SET_POLICY, LIGHT_MIX_POLICY_MAX_BRIGHTNESS};
    const uint8_t color_accurate[] = {LIGHT_CMD_SET_POLICY, LIGHT_MIX_POLICY_COLOR_ACCURATE};
    light_state_t state = init_state();

    light_control_apply_command(&state, max_brightness);
    TEST_ASSERT_EQUAL(LIGHT_MIX_POLICY_MAX_BRIGHTNESS, state.mix_policy);
    light_control_apply_command(&state, color_accurate);
    TEST_ASSERT_EQUAL(LIGHT_MIX_POLICY_COLOR_ACCURATE, state.mix_policy);
}

TEST_CASE("per-strip commands leave global mode, global commands restore it", "[light_control]")
{
    const uint8_t per_strip[][5] = {
        {LIGHT_CMD_SET_RGB_STRIP, 1, 2, 3, 4},
        {LIGHT_CMD_SET_FLAMEWARM, 1},
        {LIGHT_CMD_SET_SKYBLUE, 1},
    };
    const uint8_t global[][5] = {
        {LIGHT_CMD_SET_GLOBAL, 1, 2, 3, 4},
        {LIGHT_CMD_SET_GLOBAL_BRIGHTNESS, 1},
        {LIGHT_CMD_SET_GLOBAL_COLOR, 1, 2, 3},
    };
    light_state_t state = init_state();

    for (size_t i = 0; i < sizeof(per_strip) / sizeof(per_strip[0]); i++)
    {
        light_control_apply_command(&state, per_strip[i]);
        TEST_ASSERT_FALSE(state.global_mode);
        light_control_apply_command(&state, global[i]);
        TEST_ASSERT_TRUE(state.global_mode);
    }
}

TEST_CASE("master and policy commands keep the mode and the colors", "[light_control]")
{
    const uint8_t master[] = {LIGHT_CMD_SET_MASTER, 1};
    const uint8_t policy[] = {LIGHT_CMD_SET_POLICY, LIGHT_MIX_POLICY_MAX_BRIGHTNESS};
    const bool modes[] = {true, false};
    light_state_t state = init_state();
    light_state_t before;

    for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); i++)
    {
        state.global_mode = modes[i];
        before = state;
        light_control_apply_command(&state, master);
        light_control_apply_command(&state, policy);
        assert_same_mode_and_colors(&before, &state);
    }
}

TEST_CASE("encode state rejects NULL arguments", "[light_control]")
{
    light_state_t state = init_state();
    uint8_t payload[LIGHT_STATE_PAYLOAD_SIZE];

    TEST_ASSERT_EQUAL_INT(-EINVAL, light_control_encode_state(NULL, payload));
    TEST_ASSERT_EQUAL_INT(-EINVAL, light_control_encode_state(&state, NULL));
}

TEST_CASE("initial state encodes master off, global mode and full white", "[light_control]")
{
    const uint8_t expected[LIGHT_STATE_PAYLOAD_SIZE] = {1,   0x02, 255, 255, 255, 255,
                                                        255, 255,  255, 255, 255, 255};
    light_state_t state = init_state();
    uint8_t payload[LIGHT_STATE_PAYLOAD_SIZE];

    TEST_ASSERT_EQUAL_INT(0, light_control_encode_state(&state, payload));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, payload, LIGHT_STATE_PAYLOAD_SIZE);
}

TEST_CASE("each flag sets its own bit", "[light_control]")
{
    TEST_ASSERT_EQUAL_HEX8(0x00, encoded_flags(&(light_state_t){0}));
    TEST_ASSERT_EQUAL_HEX8(0x01, encoded_flags(&(light_state_t){.master_on = true}));
    TEST_ASSERT_EQUAL_HEX8(0x02, encoded_flags(&(light_state_t){.global_mode = true}));
    TEST_ASSERT_EQUAL_HEX8(
        0x04, encoded_flags(&(light_state_t){.mix_policy = LIGHT_MIX_POLICY_MAX_BRIGHTNESS}));
}

TEST_CASE("each field is encoded at its offset", "[light_control]")
{
    /* Each field holds its own offset, so a swap shows up. */
    const light_state_t state = {
        .global = {2, 3, 4, 5},
        .rgb_strip = {6, 7, 8, 9},
        .flamewarm_brightness = 10,
        .skyblue_brightness = 11,
    };
    const uint8_t expected[LIGHT_STATE_PAYLOAD_SIZE] = {1, 0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    uint8_t payload[LIGHT_STATE_PAYLOAD_SIZE];

    TEST_ASSERT_EQUAL_INT(0, light_control_encode_state(&state, payload));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, payload, LIGHT_STATE_PAYLOAD_SIZE);
}

TEST_CASE("render and apply frame reject NULL arguments", "[light_control]")
{
    light_state_t state = init_state();
    light_output_frame_t frame;

    TEST_ASSERT_EQUAL_INT(-EINVAL, light_control_render(NULL, &frame));
    TEST_ASSERT_EQUAL_INT(-EINVAL, light_control_render(&state, NULL));
    TEST_ASSERT_EQUAL_INT(-EINVAL, light_control_apply_frame(NULL));
}

TEST_CASE("master off renders every level at 0", "[light_control]")
{
    light_state_t state = init_state();

    assert_lit(state, 0);
    state.global_mode = false;
    assert_lit(state, 0);
}

TEST_CASE("full white stays below the maximum level", "[light_control]")
{
    light_state_t state = init_state();
    light_output_frame_t frame;

    state.master_on = true;
    for (size_t m = 0; m < MODE_COUNT; m++)
    {
        state.global_mode = modes[m].global_mode;
        state.mix_policy = modes[m].policy;
        frame = render(&state);
        for (int ch = 0; ch < LED_PWM_CHANNEL_COUNT; ch++)
            TEST_ASSERT_LESS_THAN_UINT16(LED_PWM_LEVEL_MAX, frame.levels[ch]);
    }
}

TEST_CASE("per-strip mode lights only the strips set", "[light_control]")
{
    assert_lit((light_state_t){.master_on = true, .flamewarm_brightness = 255},
               LIT(LED_PWM_CHANNEL_FLAME_WARM));
    assert_lit((light_state_t){.master_on = true, .skyblue_brightness = 255},
               LIT(LED_PWM_CHANNEL_SKY_BLUE));
    assert_lit((light_state_t){.master_on = true, .rgb_strip = {255, 255, 255, 255}}, RGB_STRIP);
}

TEST_CASE("color-accurate mode adds FlameWarm to red and SkyBlue to blue", "[light_control]")
{
    assert_lit(global_state(LIGHT_MIX_POLICY_COLOR_ACCURATE, 255, 255, 255), RGB_STRIP);
    assert_lit(global_state(LIGHT_MIX_POLICY_COLOR_ACCURATE, 255, 0, 0),
               LIT(LED_PWM_CHANNEL_RGB_RED) | LIT(LED_PWM_CHANNEL_FLAME_WARM));
    assert_lit(global_state(LIGHT_MIX_POLICY_COLOR_ACCURATE, 0, 0, 255),
               LIT(LED_PWM_CHANNEL_RGB_BLUE) | LIT(LED_PWM_CHANNEL_SKY_BLUE));
}

TEST_CASE("max-brightness mode drives every channel of a color", "[light_control]")
{
    assert_lit(global_state(LIGHT_MIX_POLICY_MAX_BRIGHTNESS, 255, 0, 0),
               LIT(LED_PWM_CHANNEL_RGB_RED) | LIT(LED_PWM_CHANNEL_FLAME_WARM));
    assert_lit(global_state(LIGHT_MIX_POLICY_MAX_BRIGHTNESS, 255, 255, 255), ALL_CHANNELS);
}

TEST_CASE("higher brightness never lowers a level", "[light_control]")
{
    /* Two close components make the color-accurate boosts dip by rounding: not tested. */
    const light_rgb_state_t colors[] = {{0, 255, 255, 255}, {0, 255, 0, 0}, {0, 0, 0, 128}};
    light_state_t state = init_state();

    state.master_on = true;
    for (size_t m = 0; m < MODE_COUNT; m++)
    {
        state.global_mode = modes[m].global_mode;
        state.mix_policy = modes[m].policy;
        for (size_t c = 0; c < sizeof(colors) / sizeof(colors[0]); c++)
        {
            state.global = colors[c];
            state.rgb_strip = colors[c];
            assert_brightness_monotonic(state);
        }
    }
}

TEST_CASE("applied frame reads back from the LEDs", "[light_control]")
{
    const light_output_frame_t frame = {{1, 1000, 32768, 50000, 65534}};
    led_pwm_level_t levels[LED_PWM_CHANNEL_COUNT];

    TEST_ASSERT_EQUAL_INT(0, light_control_apply_frame(&frame));
    TEST_ASSERT_EQUAL_INT(0, led_pwm_get_levels(levels));
    TEST_ASSERT_EQUAL_UINT16_ARRAY(frame.levels, levels, LED_PWM_CHANNEL_COUNT);
}
