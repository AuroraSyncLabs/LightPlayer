/*
** Julien ROIRON, 2026
** test_light_player.h
** File description:
** Light player test helper, shared by every test of the light player
*/

#pragma once

#include <stdint.h>

#include <lightplayer/light_player.h>

/** Number of step timestamps recorded, later steps are only counted. */
#define TEST_PLAYER_STAMPS 8

/**
 * @brief Steps run by the light player since the last test_player_setup().
 */
typedef struct test_player_s
{
    /** Number of steps run. */
    int calls;
    /** Timestamp received by each of the first TEST_PLAYER_STAMPS steps. */
    int64_t stamps_ms[TEST_PLAYER_STAMPS];
} test_player_t;

extern test_player_t test_player;

/**
 * @brief Prepare the light player for a test.
 *
 * light_player_module_init() can only be called once per app run: the first call
 * initializes the module with a step callback recording each step in test_player.
 * Every call then cancels the pending step, resets test_player and forwards the
 * next steps to step.
 *
 * @param step Step run by the light player, must not be NULL
 */
void test_player_setup(light_player_step_fn_t step);
