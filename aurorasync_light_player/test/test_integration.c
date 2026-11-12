/*
** Julien ROIRON, 2026
** test_integration.c
** File description:
** Integration tests, including the resilience scenarios of docs/quality/resilience.md
*/

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <unity.h>

#include <sdkconfig.h>

#include <lightplayer/led_pwm.h>
#include <lightplayer/light_control.h>

#define FUZZ_PAYLOADS 10000
#define FUZZ_SEED 35
#define FUZZ_MAX_LEN 8

static const uint8_t opcodes[] = {
    LIGHT_CMD_SET_MASTER,       LIGHT_CMD_SET_GLOBAL,    LIGHT_CMD_SET_GLOBAL_BRIGHTNESS,
    LIGHT_CMD_SET_GLOBAL_COLOR, LIGHT_CMD_SET_RGB_STRIP, LIGHT_CMD_SET_FLAMEWARM,
    LIGHT_CMD_SET_SKYBLUE,      LIGHT_CMD_SET_POLICY,
};

/** xorshift32: unlike rand(), gives the same payloads on every target. */
static uint32_t next_random(uint32_t *seed)
{
    *seed ^= *seed << 13;
    *seed ^= *seed >> 17;
    *seed ^= *seed << 5;
    return *seed;
}

/** Fill a random payload, starting with a known opcode half of the time, and return its length. */
static uint16_t random_payload(uint32_t *seed, uint8_t payload[FUZZ_MAX_LEN])
{
    const uint16_t len = next_random(seed) % (FUZZ_MAX_LEN + 1);

    for (uint16_t i = 0; i < len; i++)
        payload[i] = (uint8_t)next_random(seed);
    if (len > 0 && next_random(seed) % 2 == 0)
        payload[0] = opcodes[next_random(seed) % sizeof(opcodes)];
    return len;
}

TEST_CASE("random commands keep a valid state below full duty",
          "[light_control][integration][resilience]")
{
    uint32_t seed = FUZZ_SEED;
    uint8_t command[FUZZ_MAX_LEN];
    uint8_t encoded[LIGHT_STATE_PAYLOAD_SIZE];
    uint8_t after_reject[LIGHT_STATE_PAYLOAD_SIZE];
    led_pwm_level_t levels[LED_PWM_CHANNEL_COUNT];
    light_state_t state;
    int accepted = 0;

    TEST_ASSERT_EQUAL_INT(0, light_control_init(&state));
    light_control_set_master(&state, true);
    TEST_ASSERT_EQUAL_INT(0, light_control_encode_state(&state, encoded));
    for (int i = 0; i < FUZZ_PAYLOADS; i++)
    {
        const uint16_t len = random_payload(&seed, command);
        const int err = light_control_validate_command(command, len);

        if (err != 0)
        {
            TEST_ASSERT_TRUE(err == -EMSGSIZE || err == -EINVAL);
            TEST_ASSERT_EQUAL_INT(0, light_control_encode_state(&state, after_reject));
            TEST_ASSERT_EQUAL_HEX8_ARRAY(encoded, after_reject, LIGHT_STATE_PAYLOAD_SIZE);
            continue;
        }
        accepted++;
        light_control_apply_command(&state, command);
        TEST_ASSERT_EQUAL_INT(0, light_control_apply(&state));
        TEST_ASSERT_EQUAL_INT(0, led_pwm_get_levels(levels));
        for (int ch = 0; ch < LED_PWM_CHANNEL_COUNT; ch++)
            TEST_ASSERT_LESS_THAN_UINT16(LED_PWM_LEVEL_MAX, levels[ch]);
        TEST_ASSERT_EQUAL_INT(0, light_control_encode_state(&state, encoded));
        TEST_ASSERT_EQUAL_UINT8(LIGHT_STATE_PAYLOAD_VERSION, encoded[0]);
        TEST_ASSERT_BITS_LOW(0xF8, encoded[1]);
    }
    printf("%d payloads: %d accepted, %d rejected\n", FUZZ_PAYLOADS, accepted,
           FUZZ_PAYLOADS - accepted);
}

#if !CONFIG_IDF_TARGET_LINUX

#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <lightplayer/light_sequence.h>

#include "test_light_player.h"

#define SEGMENTS 3
#define SEGMENT_MS 1000
#define CUT_AT_MS 1500
#define CUT_MS 1000

/** Extra wait for the steps to run: QEMU is slower than real time. */
#define MARGIN_MS 100

static light_player_t player;
static light_state_t active;

static bool step_sequence(int64_t now_ms)
{
    return light_seq_step(&player, now_ms, &active);
}

static int64_t now_ms(void)
{
    return esp_timer_get_time() / 1000;
}

static void delay_until(int64_t at_ms)
{
    const int64_t left_ms = at_ms - now_ms();

    if (left_ms > 0)
        vTaskDelay(pdMS_TO_TICKS(left_ms));
}

TEST_CASE("sequence cut off for 1 s catches up with the clock and ends on its target",
          "[light_player][integration][resilience]")
{
    const light_state_t off = {0};
    light_seq_t seq = {.num_segments = SEGMENTS, .loops = 1};
    light_output_frame_t last_origin;
    light_output_frame_t target;
    led_pwm_level_t frozen[LED_PWM_CHANNEL_COUNT];
    led_pwm_level_t levels[LED_PWM_CHANNEL_COUNT];
    int64_t start_ms;
    int calls;

    /* Red rising to brightness 85, 170 then 255: every step changes the levels. */
    for (int i = 0; i < SEGMENTS; i++)
    {
        seq.segments[i] = (light_segment_t){
            .to = {.master_on = true,
                   .global_mode = true,
                   .global = {(uint8_t)(85 * (i + 1)), 255, 0, 0}},
            .duration_ms = SEGMENT_MS,
            .curve = EASE_LINEAR,
        };
    }
    TEST_ASSERT_EQUAL_INT(0, light_control_render(&seq.segments[SEGMENTS - 2].to, &last_origin));
    TEST_ASSERT_EQUAL_INT(0, light_control_render(&seq.segments[SEGMENTS - 1].to, &target));
    test_player_setup(step_sequence);
    start_ms = now_ms();
    TEST_ASSERT_EQUAL_INT(0, light_seq_start(&player, &seq, &off, start_ms));
    light_player_schedule();

    delay_until(start_ms + CUT_AT_MS);
    light_player_cancel();
    calls = test_player.calls;
    TEST_ASSERT_EQUAL_INT(0, led_pwm_get_levels(frozen));
    vTaskDelay(pdMS_TO_TICKS(CUT_MS));
    TEST_ASSERT_EQUAL_INT(calls, test_player.calls);
    TEST_ASSERT_EQUAL_INT(0, led_pwm_get_levels(levels));
    TEST_ASSERT_EQUAL_UINT16_ARRAY(frozen, levels, LED_PWM_CHANNEL_COUNT);

    light_player_schedule();
    vTaskDelay(pdMS_TO_TICKS(MARGIN_MS));
    TEST_ASSERT_EQUAL_INT(0, led_pwm_get_levels(levels));
    /* Above the start of the last segment: the rest of the second one was skipped. */
    TEST_ASSERT_GREATER_THAN_UINT16(last_origin.levels[LED_PWM_CHANNEL_RGB_RED],
                                    levels[LED_PWM_CHANNEL_RGB_RED]);

    delay_until(start_ms + (SEGMENTS * SEGMENT_MS) + MARGIN_MS);
    TEST_ASSERT_FALSE(light_seq_is_active(&player));
    TEST_ASSERT_EQUAL_INT(0, led_pwm_get_levels(levels));
    TEST_ASSERT_EQUAL_UINT16_ARRAY(target.levels, levels, LED_PWM_CHANNEL_COUNT);
}

#endif
