/*
** led_pwm.c
** File description:
** PWM LED control API, common to every backend.
*/

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <freertos/FreeRTOS.h>

#include <lightplayer/led_pwm.h>

#include "led_pwm_port.h"

static bool initialized = false;
static led_pwm_level_t applied_levels[LED_PWM_CHANNEL_COUNT];
static portMUX_TYPE applied_levels_lock = portMUX_INITIALIZER_UNLOCKED;

/* Drive count channels from first, then record the levels the backend accepted in one
   critical section, so a reader never gets half of a frame. */
static int apply_levels(size_t first, size_t count, const led_pwm_level_t *levels)
{
    size_t applied;
    int err = 0;

    for (applied = 0; applied < count; applied++)
    {
        err = led_pwm_port_apply((led_pwm_channel_t)(first + applied), levels[applied]);
        if (err != 0)
            break;
    }
    portENTER_CRITICAL(&applied_levels_lock);
    memcpy(&applied_levels[first], levels, applied * sizeof(*levels));
    portEXIT_CRITICAL(&applied_levels_lock);
    return err;
}

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
    return apply_levels(channel, 1, &level);
}

int led_pwm_set_levels(const led_pwm_level_t levels[LED_PWM_CHANNEL_COUNT])
{
    if (levels == NULL)
        return -EINVAL;
    if (!initialized)
        return -EAGAIN;
    return apply_levels(0, LED_PWM_CHANNEL_COUNT, levels);
}

int led_pwm_get_levels(led_pwm_level_t levels[LED_PWM_CHANNEL_COUNT])
{
    if (levels == NULL)
        return -EINVAL;
    if (!initialized)
        return -EAGAIN;
    portENTER_CRITICAL(&applied_levels_lock);
    memcpy(levels, applied_levels, sizeof(applied_levels));
    portEXIT_CRITICAL(&applied_levels_lock);
    return 0;
}

int led_pwm_off(void)
{
    const led_pwm_level_t off_levels[LED_PWM_CHANNEL_COUNT] = {0};

    return led_pwm_set_levels(off_levels);
}
