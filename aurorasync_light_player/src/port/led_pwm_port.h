/*
** Julien ROIRON, 2026
** led_pwm_port.h
** File description:
** Include file containing the PWM LED backend interface
*/

#pragma once

#include <lightplayer/led_pwm.h>

/**
 * @brief Initialize the backend outputs.
 *
 * @return 0 on success, negative errno on failure
 */
int led_pwm_port_init(void);

/**
 * @brief Drive one output.
 *
 * @param channel Channel to control, already validated
 * @param level Brightness from LED_PWM_LEVEL_MIN to LED_PWM_LEVEL_MAX
 * @return 0 on success, negative errno on failure
 */
int led_pwm_port_apply(led_pwm_channel_t channel, led_pwm_level_t level);
