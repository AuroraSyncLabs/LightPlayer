/*
** Julien ROIRON, 2026
** main.c
** File description:
** Source file containing the main entry of the example.
*/

#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <lightplayer/light_control.h>

void app_main(void)
{
    light_state_t light_state;

    printf("Basic blink Example!\n");

    // Initialize PWM module.
    int result_pwm = led_pwm_init();
    if (result_pwm < 0)
    {
        printf("led_pwm_init failed: %d\n", result_pwm);
        exit(result_pwm);
    }

    // Initialize the light state variable.
    int result_light = light_control_init(&light_state);
    if (result_light < 0)
    {
        printf("light_control_init failed: %d\n", result_light);
        exit(result_light);
    }

    while (true)
    {
        // We turn the light on.
        light_control_set_master(&light_state, true);
        // We set the light to white at the maximum intensity.
        light_control_set_global(&light_state, 255, 255, 255, 255);
        // We apply the light state to the lights.
        light_control_apply(&light_state);

        printf("Keep it on for 1 sec.\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // We turn the light off.
        light_control_set_master(&light_state, false);
        // We apply the light state to the lights again to turn it off.
        light_control_apply(&light_state);

        printf("Keep it off for 1 sec.\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
