/*
** Julien ROIRON, 2026
** light_player.h
** File description:
** Include file containing the light sequence player scheduler
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Callback executing one step of the light sequence player.
 *
 * @param now_ms Current monotonic timestamp in milliseconds.
 * @return true if another step is needed, false if the sequence is done.
 */
typedef bool (*light_player_step_fn_t)(int64_t now_ms);

/**
 * @brief Create the timer driving the light sequence player.
 *
 * @param step Callback run on every timer tick, must not be NULL.
 */
void light_player_module_init(light_player_step_fn_t step);

/**
 * @brief Run a player step as soon as possible.
 *
 * Arming an already armed player simply moves the next step to now, so this is
 * safe to call whenever a sequence is started or asked to stop.
 */
void light_player_schedule(void);

/**
 * @brief Cancel the pending player step.
 *
 * Only the controller that cleared the player may cancel it, otherwise a
 * running sequence would be frozen mid-way with no one left to step it.
 */
void light_player_cancel(void);
