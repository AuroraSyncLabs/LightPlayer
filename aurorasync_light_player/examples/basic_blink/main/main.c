/*
** Julien ROIRON, 2026
** main.c
** File description:
** Source file containing the main entry of the exemple.
*/

#include <stdio.h>
#include <inttypes.h>
#include <esp_system.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <lightplayer/light_control.h>

void app_main(void)
{
    light_state_t light_state;

    printf("Basic blink Exemple!\n");

    // Initialize PWM module.
    led_pwm_init();

    // Initialize the light state variable.
    light_control_init(&light_state);

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
