/*
** Julien ROIRON, 2026
** main.c
** File description:
** Source file containing the main entry of the exemple.
*/

#include <esp_system.h>
#include <inttypes.h>
#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <lightplayer/light_control.h>

#include "esp_timer.h"
#include "lightplayer/light_player.h"
#include "lightplayer/light_sequence.h"

light_player_t light_player;
light_state_t device_light_state;

static bool light_player_step_fn(int64_t now_ms)
{
    /** Execute sequence step. The function must return true if the sequence must continue. **/
    return light_seq_step(&light_player, now_ms, &device_light_state);
}

void app_main(void)
{
    light_state_t light_state_off = {0};
    light_state_t light_state_red = {0};
    int64_t now;

    printf("Ramp-up Hold Ramp-down Exemple!\n");

    // Initialize PWM module.
    if (led_pwm_init() < 0)
        exit(1);

    // Initialize the light state variables.
    light_control_init(&light_state_off);
    light_control_init(&light_state_red);

    // Configure the light states
    light_control_set_master(&light_state_off, false);
    light_control_set_master(&light_state_red, true);
    light_control_set_global(&light_state_red, 255, 255, 0, 0);

    // Initialize the light player module with step function.
    light_player_module_init(light_player_step_fn);

    // We initialize the sequence.
    light_seq_t light_sequence = {0};
    light_sequence.loops = 1;
    light_sequence.num_segments = 3;
    // Off to red in 1000ms.
    light_sequence.segments[0].curve = EASE_SMOOTH;
    light_sequence.segments[0].duration_ms = 1000;
    light_sequence.segments[0].to = light_state_red;
    // Hold red for 1000ms.
    light_sequence.segments[1].curve = EASE_LINEAR;
    light_sequence.segments[1].duration_ms = 1000;
    light_sequence.segments[1].to = light_state_red;
    // Red to off in 1000ms.
    light_sequence.segments[2].curve = EASE_RELEASE;
    light_sequence.segments[2].duration_ms = 1000;
    light_sequence.segments[2].to = light_state_off;

    // Start the sequence and schedule.
    now = esp_timer_get_time() / 1000;
    light_seq_start(&light_player, &light_sequence, &light_state_off, now);
    light_player_schedule();

    // We wait 5 seconds until it finishes (the light sequence last for 3 seconds)
    printf("Wait 5000ms until it finishes.\n");
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
