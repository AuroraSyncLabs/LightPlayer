/*
** Julien ROIRON, 2026
** test_light_player.c
** File description:
** Unit tests of the light player scheduler, on ESP targets only (esp_timer)
*/

#include <sdkconfig.h>

#if !CONFIG_IDF_TARGET_LINUX

#include <limits.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <unity.h>

#include "test_light_player.h"

/** Interval between two steps, set by light_player.c. */
#define STEP_INTERVAL_MS 10

/** Extra wait for the steps to run: QEMU is slower than real time. */
#define MARGIN_MS 100

test_player_t test_player;

static light_player_step_fn_t forwarded_step;
static int steps_to_continue;

static bool record_step(int64_t now_ms)
{
    if (test_player.calls < TEST_PLAYER_STAMPS)
        test_player.stamps_ms[test_player.calls] = now_ms;
    test_player.calls++;
    return forwarded_step(now_ms);
}

void test_player_setup(light_player_step_fn_t step)
{
    static bool initialized = false;

    if (!initialized)
    {
        light_player_module_init(record_step);
        initialized = true;
    }
    light_player_cancel();
    test_player = (test_player_t){0};
    forwarded_step = step;
}

/** Ask for another step steps_to_continue times, then stop. */
static bool continue_n_times(int64_t now_ms)
{
    (void)now_ms;
    return test_player.calls <= steps_to_continue;
}

TEST_CASE("schedule runs the step within a few ticks", "[light_player]")
{
    test_player_setup(continue_n_times);
    steps_to_continue = 0;
    light_player_schedule();
    vTaskDelay(pdMS_TO_TICKS(MARGIN_MS));
    TEST_ASSERT_EQUAL_INT(1, test_player.calls);
}

TEST_CASE("step continuing N times runs N + 1 times, 10 ms apart", "[light_player]")
{
    const int n = 3;

    test_player_setup(continue_n_times);
    steps_to_continue = n;
    light_player_schedule();
    vTaskDelay(pdMS_TO_TICKS((n * STEP_INTERVAL_MS) + MARGIN_MS));
    TEST_ASSERT_EQUAL_INT(n + 1, test_player.calls);
    for (int i = 1; i <= n; i++)
    {
        TEST_ASSERT_GREATER_OR_EQUAL_INT(
            STEP_INTERVAL_MS, (int)(test_player.stamps_ms[i] - test_player.stamps_ms[i - 1]));
    }
}

TEST_CASE("cancel stops the pending steps", "[light_player]")
{
    int calls;

    test_player_setup(continue_n_times);
    steps_to_continue = INT_MAX;
    light_player_schedule();
    vTaskDelay(pdMS_TO_TICKS(MARGIN_MS));
    light_player_cancel();
    calls = test_player.calls;
    TEST_ASSERT_GREATER_THAN_INT(1, calls);
    vTaskDelay(pdMS_TO_TICKS(MARGIN_MS));
    TEST_ASSERT_EQUAL_INT(calls, test_player.calls);
}

#endif
