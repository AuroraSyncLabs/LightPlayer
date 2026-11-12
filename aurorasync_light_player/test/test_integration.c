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
