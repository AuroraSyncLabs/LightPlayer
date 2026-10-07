/*
** Quentin FILLIAERT, 2026
** light_control.h
** File description:
** Include file containing the logical light control API
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <lightplayer/led_pwm.h>

// Light command definition
#define LIGHT_CMD_SET_MASTER 0x01
#define LIGHT_CMD_SET_GLOBAL 0x02
#define LIGHT_CMD_SET_GLOBAL_BRIGHTNESS 0x03
#define LIGHT_CMD_SET_GLOBAL_COLOR 0x04
#define LIGHT_CMD_SET_RGB_STRIP 0x10
#define LIGHT_CMD_SET_FLAMEWARM 0x11
#define LIGHT_CMD_SET_SKYBLUE 0x12
#define LIGHT_CMD_SET_POLICY 0x20

/**
 * @brief Version byte used by the BLE LIGHT_STATE payload.
 */
#define LIGHT_STATE_PAYLOAD_VERSION 1

/**
 * @brief Fixed byte length of the BLE LIGHT_STATE payload.
 */
#define LIGHT_STATE_PAYLOAD_SIZE 12

/**
 * @brief Policy used when global RGB mode is mixed into the five PWM outputs.
 */
typedef enum light_mix_policy_e
{
    /** Preserve the requested color balance and use duplicated channels only for dominant red/blue.
     */
    LIGHT_MIX_POLICY_COLOR_ACCURATE = 0,
    /** Drive duplicated red/blue channels aggressively for higher output. */
    LIGHT_MIX_POLICY_MAX_BRIGHTNESS = 1,
} light_mix_policy_t;

/**
 * @brief Brightness and color tuple used by global mode and the physical RGB strip.
 */
typedef struct light_rgb_state_s
{
    /** Scalar brightness, 0 means off and 255 means full requested output. */
    uint8_t brightness;
    /** Red component, 0..255. */
    uint8_t r;
    /** Green component, 0..255. */
    uint8_t g;
    /** Blue component, 0..255. */
    uint8_t b;
} light_rgb_state_t;

/**
 * @brief Rendered physical PWM output for every light channel.
 *
 * This is the correct animation surface: logical fields such as master_on,
 * global_mode, and mix_policy are discrete, while PWM levels can be interpolated
 * safely for transitions.
 */
typedef struct light_output_frame_s
{
    led_pwm_level_t levels[LED_PWM_CHANNEL_COUNT];
} light_output_frame_t;

/**
 * @brief Complete logical light state stored by the device.
 *
 * FlameWarm and SkyBlue are intentionally brightness-only because each one is
 * a single connected color channel. Global mode still accepts RGB because it
 * represents the desired whole-fixture color before firmware mixing.
 */
typedef struct light_state_s
{
    /** Master output state; false forces all PWM outputs to zero without clearing stored settings.
     */
    bool master_on;
    /** True uses global RGB mode; false uses independent per-strip settings. */
    bool global_mode;
    /** Global mode red/blue booster policy. */
    light_mix_policy_t mix_policy;
    /** Desired whole-fixture brightness and color. */
    light_rgb_state_t global;
    /** Independent physical RGB strip brightness and color. */
    light_rgb_state_t rgb_strip;
    /** Independent FlameWarm single-channel brightness. */
    uint8_t flamewarm_brightness;
    /** Independent SkyBlue single-channel brightness. */
    uint8_t skyblue_brightness;
} light_state_t;

/**
 * @brief Initialize logical state with defaults and apply the initial PWM output.
 *
 * The default state is master off, global mode enabled, full white stored, and
 * the default mix policy selected by Kconfig.
 *
 * @param state State to initialize
 * @return 0 on success, negative errno on failure
 */
int light_control_init(light_state_t *state);

/**
 * @brief Recompute PWM levels from the existing logical state.
 *
 * @param state State to apply
 * @return 0 on success, negative errno on failure
 */
int light_control_apply(const light_state_t *state);

/**
 * @brief Render a logical light state into physical PWM output levels.
 *
 * This does not touch hardware.
 *
 * @param state Logical state to render
 * @param frame Output physical PWM frame
 * @return 0 on success, negative errno on failure
 */
int light_control_render(const light_state_t *state, light_output_frame_t *frame);

/**
 * @brief Apply a pre-rendered physical PWM output frame.
 *
 * @param frame Physical PWM frame to apply
 * @return 0 on success, negative errno on failure
 */
int light_control_apply_frame(const light_output_frame_t *frame);

/**
 * @brief Set master on/off without changing stored brightness or color values.
 */
void light_control_set_master(light_state_t *state, bool on);

/**
 * @brief Set whole-fixture global brightness/color and switch to global mode.
 *
 * This does not change master_on. Use light_control_set_master() for power.
 */
void light_control_set_global(light_state_t *state, uint8_t brightness, uint8_t r, uint8_t g,
                              uint8_t b);

/**
 * @brief Set whole-fixture global brightness and switch to global mode.
 */
void light_control_set_global_brightness(light_state_t *state, uint8_t brightness);

/**
 * @brief Set whole-fixture global color and switch to global mode.
 */
void light_control_set_global_color(light_state_t *state, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Set the physical RGB strip and switch to per-strip mode.
 */
void light_control_set_rgb_strip(light_state_t *state, uint8_t brightness, uint8_t r, uint8_t g,
                                 uint8_t b);

/**
 * @brief Set the FlameWarm single-channel brightness and switch to per-strip mode.
 */
void light_control_set_flamewarm(light_state_t *state, uint8_t brightness);

/**
 * @brief Set the SkyBlue single-channel brightness and switch to per-strip mode.
 */
void light_control_set_skyblue(light_state_t *state, uint8_t brightness);

/**
 * @brief Set the global-mode mixing policy.
 */
void light_control_set_mix_policy(light_state_t *state, light_mix_policy_t policy);

/**
 * @brief Encode the current logical state into the fixed BLE LIGHT_STATE payload.
 *
 * @param state State to encode
 * @param payload Output buffer; must be LIGHT_STATE_PAYLOAD_SIZE bytes
 * @return 0 on success, negative errno on failure
 */
int light_control_encode_state(const light_state_t *state,
                               uint8_t payload[LIGHT_STATE_PAYLOAD_SIZE]);

/**
 * @brief Validate a LIGHT_COMMAND payload: opcode, length and enum-like fields.
 *
 * @param command LIGHT_COMMAND payload.
 * @param len Payload length.
 * @return 0 when valid, -EMSGSIZE on a wrong length, -EINVAL on an unknown
 *         opcode or an out-of-range value.
 */
int light_control_validate_command(const uint8_t *command, uint16_t len);

/**
 * @brief Apply a validated command to the logical light state.
 *
 * This function assumes light_control_validate_command() has already accepted
 * the payload.
 *
 * @param state Logical light state to mutate.
 * @param command Validated LIGHT_COMMAND payload.
 */
void light_control_apply_command(light_state_t *state, const uint8_t *command);
