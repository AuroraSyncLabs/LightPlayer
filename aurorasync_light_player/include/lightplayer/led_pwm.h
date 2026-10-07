/*
** Quentin FILLIAERT, 2026
** led_pwm.h
** File description:
** Include file containing the PWM LED control API
*/

#pragma once

#include <stdint.h>

#define LED_PWM_LEVEL_MIN 0
#define LED_PWM_LEVEL_MAX UINT16_MAX

typedef uint16_t led_pwm_level_t;

/**
 * @brief PWM controlled LED channels.
 */
typedef enum led_pwm_channel_e
{
    LED_PWM_CHANNEL_RGB_RED = 0,
    LED_PWM_CHANNEL_RGB_GREEN,
    LED_PWM_CHANNEL_RGB_BLUE,
    LED_PWM_CHANNEL_FLAME_WARM,
    LED_PWM_CHANNEL_SKY_BLUE,
    LED_PWM_CHANNEL_COUNT,
} led_pwm_channel_t;

/**
 * @brief Initialize the PWM LED outputs and switch them off.
 *
 * @return 0 on success, negative errno on failure
 */
int led_pwm_init(void);

/**
 * @brief Set one PWM LED channel.
 *
 * @param channel Channel to control
 * @param level Brightness from LED_PWM_LEVEL_MIN to LED_PWM_LEVEL_MAX
 * @return 0 on success, negative errno on failure
 */
int led_pwm_set_channel(led_pwm_channel_t channel, led_pwm_level_t level);

/**
 * @brief Set every PWM LED channel.
 *
 * The level array order is: RGB red, RGB green, RGB blue, flame warm, sky blue.
 *
 * @param levels Five brightness values from LED_PWM_LEVEL_MIN to LED_PWM_LEVEL_MAX
 * @return 0 on success, negative errno on failure
 */
int led_pwm_set_levels(const led_pwm_level_t levels[LED_PWM_CHANNEL_COUNT]);

/**
 * @brief Switch off every PWM LED channel.
 *
 * @return 0 on success, negative errno on failure
 */
int led_pwm_off(void);
