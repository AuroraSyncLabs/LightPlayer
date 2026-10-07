/*
** led_pwm.c
** File description:
** PWM LED control API, ESP32-C6 LEDC backend.
*/

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <driver/ledc.h>
#include <esp_err.h>
#include <esp_log.h>

#include <lightplayer/led_pwm.h>

static const char *TAG = "LightPlayer_LED_PWM";

#define LED_PWM_SPEED_MODE LEDC_LOW_SPEED_MODE
#define LED_PWM_TIMER LEDC_TIMER_0
/* PLL-derived source clock used by LEDC for these frequencies on the ESP32-C6. */
#define LED_PWM_SRC_CLK_HZ 80000000U
/* Cap the duty resolution so the 4 kHz default keeps comfortable headroom. The
   original nRF firmware used 16-bit duty at 4 kHz, which is unreachable on LEDC
   (would need a 262 MHz clock); the led_pwm.h uint16 interface is preserved and
   the level is scaled into the available duty range. */
#define LED_PWM_MAX_RES_BIT LEDC_TIMER_13_BIT

static const int led_gpios[LED_PWM_CHANNEL_COUNT] = {
    [LED_PWM_CHANNEL_RGB_RED] = CONFIG_LIGHT_PLAYER_PWM_GPIO_RGB_RED,
    [LED_PWM_CHANNEL_RGB_GREEN] = CONFIG_LIGHT_PLAYER_PWM_GPIO_RGB_GREEN,
    [LED_PWM_CHANNEL_RGB_BLUE] = CONFIG_LIGHT_PLAYER_PWM_GPIO_RGB_BLUE,
    [LED_PWM_CHANNEL_FLAME_WARM] = CONFIG_LIGHT_PLAYER_PWM_GPIO_FLAME_WARM,
    [LED_PWM_CHANNEL_SKY_BLUE] = CONFIG_LIGHT_PLAYER_PWM_GPIO_SKY_BLUE,
};

static const ledc_channel_t led_channels[LED_PWM_CHANNEL_COUNT] = {
    [LED_PWM_CHANNEL_RGB_RED] = LEDC_CHANNEL_0,  [LED_PWM_CHANNEL_RGB_GREEN] = LEDC_CHANNEL_1,
    [LED_PWM_CHANNEL_RGB_BLUE] = LEDC_CHANNEL_2, [LED_PWM_CHANNEL_FLAME_WARM] = LEDC_CHANNEL_3,
    [LED_PWM_CHANNEL_SKY_BLUE] = LEDC_CHANNEL_4,
};

static bool initialized = false;
static uint32_t max_duty = 0;

/* Highest duty resolution (bits) supported for freq_hz against the source clock,
   capped at LED_PWM_MAX_RES_BIT. */
static ledc_timer_bit_t pick_resolution(uint32_t freq_hz)
{
    for (int bits = LED_PWM_MAX_RES_BIT; bits > 0; --bits)
        if (((uint64_t)freq_hz << bits) <= LED_PWM_SRC_CLK_HZ)
            return (ledc_timer_bit_t)bits;
    return (ledc_timer_bit_t)1;
}

/* Map a 16-bit logical level (0..LED_PWM_LEVEL_MAX) onto the LEDC duty range.
   Full scale maps to max_duty = (2^res - 1), so the output never reaches a
   constant-high state. */
static uint32_t level_to_duty(led_pwm_level_t level)
{
    return (uint32_t)(((uint64_t)level * max_duty) / LED_PWM_LEVEL_MAX);
}

static int apply_channel(led_pwm_channel_t channel, led_pwm_level_t level)
{
    const uint32_t duty = level_to_duty(level);

    esp_err_t err = ledc_set_duty(LED_PWM_SPEED_MODE, led_channels[channel], duty);
    if (err == ESP_OK)
        err = ledc_update_duty(LED_PWM_SPEED_MODE, led_channels[channel]);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "PWM ch=%u level=%u duty=%u failed: %s", channel, level, (unsigned)duty,
                 esp_err_to_name(err));
        return -EIO;
    }
    return 0;
}

int led_pwm_init(void)
{
    const ledc_timer_bit_t res = pick_resolution(CONFIG_LIGHT_PLAYER_PWM_FREQ_HZ);
    max_duty = (1U << res) - 1U;

    const ledc_timer_config_t timer = {
        .speed_mode = LED_PWM_SPEED_MODE,
        .timer_num = LED_PWM_TIMER,
        .duty_resolution = res,
        .freq_hz = CONFIG_LIGHT_PLAYER_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t err = ledc_timer_config(&timer);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "ledc_timer_config failed: %s", esp_err_to_name(err));
        return -EIO;
    }

    for (size_t i = 0; i < LED_PWM_CHANNEL_COUNT; i++)
    {
        const ledc_channel_config_t ch = {
            .speed_mode = LED_PWM_SPEED_MODE,
            .channel = led_channels[i],
            .timer_sel = LED_PWM_TIMER,
            .intr_type = LEDC_INTR_DISABLE,
            .gpio_num = led_gpios[i],
            .duty = 0,
            .hpoint = 0,
        };
        err = ledc_channel_config(&ch);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "ledc_channel_config ch=%zu gpio=%d failed: %s", i, led_gpios[i],
                     esp_err_to_name(err));
            return -EIO;
        }
    }

    initialized = true;
    ESP_LOGI(TAG, "PWM LED API initialized (%u Hz, %u-bit, max_duty=%u)",
             (unsigned)CONFIG_LIGHT_PLAYER_PWM_FREQ_HZ, (unsigned)res, (unsigned)max_duty);
    return led_pwm_off();
}

int led_pwm_set_channel(led_pwm_channel_t channel, led_pwm_level_t level)
{
    if (!initialized)
        return -EAGAIN;
    if ((int)channel < 0 || channel >= LED_PWM_CHANNEL_COUNT)
        return -EINVAL;
    return apply_channel(channel, level);
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
        err = apply_channel((led_pwm_channel_t)i, levels[i]);
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
