/*
** led_pwm.c
** File description:
** PWM LED control API, common to every backend.
*/

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>

#include <lightplayer/led_pwm.h>

#include "led_pwm_port.h"

static bool initialized = false;

int led_pwm_init(void)
{
    const int err = led_pwm_port_init();

    if (err != 0)
        return err;
    initialized = true;
    return led_pwm_off();
}

int led_pwm_set_channel(led_pwm_channel_t channel, led_pwm_level_t level)
{
    if (!initialized)
        return -EAGAIN;
    if ((int)channel < 0 || channel >= LED_PWM_CHANNEL_COUNT)
        return -EINVAL;
    return led_pwm_port_apply(channel, level);
}

int led_pwm_set_levels(const led_pwm_level_t levels[LED_PWM_CHANNEL_COUNT])
{
    int err;

    if (levels == NULL)
        return -EINVAL;
    if (!initialized)
        return -EAGAIN;
    for (size_t i = 0; i < LED_PWM_CHANNEL_COUNT; i++)
    {
        err = led_pwm_port_apply((led_pwm_channel_t)i, levels[i]);
        if (err != 0)
            return err;
    }
    return 0;
}

int led_pwm_off(void)
{
    const led_pwm_level_t off_levels[LED_PWM_CHANNEL_COUNT] = {0};

    return led_pwm_set_levels(off_levels);
}
