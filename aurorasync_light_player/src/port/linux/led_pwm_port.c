/*
** Julien ROIRON, 2026
** led_pwm_port.c
** File description:
** PWM LED backend, emulated on the linux target.
*/

#include <esp_log.h>

#include "led_pwm_port.h"

static const char *TAG = "LightPlayer_LED_PWM";

int led_pwm_port_init(void)
{
    ESP_LOGI(TAG, "PWM LED API initialized (emulated)");
    return 0;
}

int led_pwm_port_apply(led_pwm_channel_t channel, led_pwm_level_t level)
{
    ESP_LOGD(TAG, "PWM ch=%u level=%u", channel, level);
    return 0;
}
