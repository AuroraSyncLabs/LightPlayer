/*
** Julien ROIRON, 2026
** test_led_pwm.c
** File description:
** Unit tests of the PWM LED API, led_pwm_init() is run by setUp()
*/

#include <errno.h>
#include <unity.h>

#include <lightplayer/led_pwm.h>

static const led_pwm_level_t frame[LED_PWM_CHANNEL_COUNT] = {1, 1000, 32768, 50000,
                                                             LED_PWM_LEVEL_MAX};

TEST_CASE("get levels returns the levels set", "[led_pwm]")
{
    led_pwm_level_t levels[LED_PWM_CHANNEL_COUNT];

    TEST_ASSERT_EQUAL_INT(0, led_pwm_set_levels(frame));
    TEST_ASSERT_EQUAL_INT(0, led_pwm_get_levels(levels));
    TEST_ASSERT_EQUAL_UINT16_ARRAY(frame, levels, LED_PWM_CHANNEL_COUNT);
}

TEST_CASE("set channel only changes its level", "[led_pwm]")
{
    led_pwm_level_t expected[LED_PWM_CHANNEL_COUNT] = {0};
    led_pwm_level_t levels[LED_PWM_CHANNEL_COUNT];

    expected[LED_PWM_CHANNEL_FLAME_WARM] = 1234;
    TEST_ASSERT_EQUAL_INT(0, led_pwm_off());
    TEST_ASSERT_EQUAL_INT(0, led_pwm_set_channel(LED_PWM_CHANNEL_FLAME_WARM, 1234));
    TEST_ASSERT_EQUAL_INT(0, led_pwm_get_levels(levels));
    TEST_ASSERT_EQUAL_UINT16_ARRAY(expected, levels, LED_PWM_CHANNEL_COUNT);
}

TEST_CASE("off resets every level", "[led_pwm]")
{
    const led_pwm_level_t expected[LED_PWM_CHANNEL_COUNT] = {0};
    led_pwm_level_t levels[LED_PWM_CHANNEL_COUNT];

    TEST_ASSERT_EQUAL_INT(0, led_pwm_set_levels(frame));
    TEST_ASSERT_EQUAL_INT(0, led_pwm_off());
    TEST_ASSERT_EQUAL_INT(0, led_pwm_get_levels(levels));
    TEST_ASSERT_EQUAL_UINT16_ARRAY(expected, levels, LED_PWM_CHANNEL_COUNT);
}

TEST_CASE("rejected calls keep the levels", "[led_pwm]")
{
    led_pwm_level_t levels[LED_PWM_CHANNEL_COUNT];

    TEST_ASSERT_EQUAL_INT(0, led_pwm_set_levels(frame));
    TEST_ASSERT_EQUAL_INT(-EINVAL, led_pwm_set_channel(LED_PWM_CHANNEL_COUNT, 0));
    TEST_ASSERT_EQUAL_INT(-EINVAL, led_pwm_set_levels(NULL));
    TEST_ASSERT_EQUAL_INT(0, led_pwm_get_levels(levels));
    TEST_ASSERT_EQUAL_UINT16_ARRAY(frame, levels, LED_PWM_CHANNEL_COUNT);
}

TEST_CASE("get levels rejects a NULL array", "[led_pwm]")
{
    TEST_ASSERT_EQUAL_INT(-EINVAL, led_pwm_get_levels(NULL));
}
