// File: aurorasync_light_player/src/light_player.c (Size: 1527 bytes, SHA: 93b3f73288f5fc3aac4a0123e7ff2284c9398007)

/*
** Julien ROIRON, 2026
** light_player.c
** File description:
** Source file driving the single light sequence player of the device
*/

#include <assert.h>

#include <esp_log.h>
#include <esp_timer.h>

#include <lightplayer/light_player.h>

static const char *TAG = "LightPlayer";

#define LOG_INF(...) ESP_LOGI(TAG, __VA_ARGS__)

/** The player runs at 100 updates/s, fast enough for a fade the eye reads as smooth. */
#define LIGHT_PLAYER_UPDATE_INTERVAL_US (10 * 1000)

#define GET_MONOTONIC_MS (esp_timer_get_time() / 1000)

static esp_timer_handle_t light_player_timer;
static light_player_step_fn_t light_player_step;

/**
 * @brief Step the player and re-arm the timer until the sequence is over.
 *
 * @param arg Unused
 */
static void light_player_timer_cb(void *arg)
{
    int64_t now = GET_MONOTONIC_MS;

    (void)arg;
    if (light_player_step(now))
    {
        esp_timer_start_once(light_player_timer, LIGHT_PLAYER_UPDATE_INTERVAL_US);
    }
}

void light_player_module_init(light_player_step_fn_t step)
{
    const esp_timer_create_args_t args = {
        .callback = light_player_timer_cb,
        .name = "light_player",
    };

    assert(step != NULL);
    light_player_step = step;
    ESP_ERROR_CHECK(esp_timer_create(&args, &light_player_timer));
    LOG_INF("Light player module initialized");
}

void light_player_schedule(void)
{
    if (light_player_timer == NULL)
    {
        ESP_LOGE(TAG, "light_player_schedule() called before light_player_module_init()");
        return;
    }
    esp_timer_stop(light_player_timer);
    esp_timer_start_once(light_player_timer, 0);
}

void light_player_cancel(void)
{
    if (light_player_timer == NULL)
    {
        ESP_LOGE(TAG, "light_player_cancel() called before light_player_module_init()");
        return;
    }
    esp_timer_stop(light_player_timer);
}
