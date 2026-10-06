/*
** Julien ROIRON, 2026
** light_sequence.h
** File description:
** Include file representing light sequence for animation
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <lightplayer/easing.h>
#include <lightplayer/light_control.h>

/**
 * Maximum number of segments = 4
 * - For alarm 3
 * - For meditation 4 <-
 */
#define LIGHT_SEQ_MAX_SEGMENTS 4

/**
 * @brief Enumeration representing the runtime phase of an active light sequence.
 */
typedef enum light_seq_phase_s
{
    LIGHT_SEQ_IDLE,                /**< No sequence is active. */
    LIGHT_SEQ_RUNNING,             /**< The sequence is running. */
    LIGHT_SEQ_STOP_KEEP_CURRENT,   /**< Stop without modifying the light configuration. */
    LIGHT_SEQ_STOP_ROLLBACK_START, /**< Stop and rollback the light configuration to its initial
                                      state. */
    LIGHT_SEQ_STOP_TO_TARGET,      /**< Stop and set the light configuration to the target one. */
} light_seq_phase_t;

/**
 * @brief Structure representing a light segment.
 */
typedef struct light_segment_s
{
    light_state_t to;     /**< Target light state to transition to. */
    uint32_t duration_ms; /**< Duration of the transition, in milliseconds. */
    ease_curve_t curve;   /**< Easing curve used for the transition. */
} light_segment_t;

/**
 * @brief Structure representing a light sequence.
 */
typedef struct light_seq_s
{
    light_segment_t segments[LIGHT_SEQ_MAX_SEGMENTS]; /**< Segments array. */
    uint8_t num_segments;                             /**< Number of segments (worst case: 4). */
    uint32_t loops;                                   /**< Number of loops to execute.
  0=infinite, 1=one-shot */
    light_state_t origin;                             /**< The original light state. */
} light_seq_t;

/**
 * @brief Structure representing a light player.
 */
typedef struct light_player_s
{
    light_seq_t sequence;               /**< The current light sequence. */
    light_state_t segment_origin;       /**< The original light state of the segment. */
    light_seq_phase_t phase;            /**< The current phase. */
    uint8_t current_segment;            /**< The current segment of the sequence. */
    uint32_t loop_done;                 /**< The number of loops completed. */
    int64_t segment_start_ms;           /**< The start tick of the segment in ms. */
    light_output_frame_t origin_frame;  /**< Rendered frame of segment_origin. */
    light_output_frame_t target_frame;  /**< Rendered frame of the segment target. */
    light_output_frame_t current_frame; /**< The current computed frame. */
} light_player_t;

/**
 * @brief Reset a player to the idle phase.
 *
 * @param player Player to clear.
 */
void light_seq_clear(light_player_t *player);

/**
 * @brief Load a sequence into a player and arm it.
 *
 * The sequence starts from the given light state, which is also stored as the
 * rollback origin. A sequence with no segment, or whose segments all have a
 * null duration, is rejected and leaves the player idle.
 *
 * @param player Player to arm.
 * @param sequence Sequence to play.
 * @param current The light state the sequence starts from.
 * @param now_ms Current timestamp in milliseconds.
 * @return 0 on success, negative errno on failure
 */
int light_seq_start(light_player_t *player, const light_seq_t *sequence,
                    const light_state_t *current, int64_t now_ms);

/**
 * @brief Execute one player step and apply the rendered PWM frame.
 *
 * The logical light state is only updated on segment boundaries, so a caller
 * reading it never observes an intermediate interpolated value.
 *
 * @param player Player to advance.
 * @param now_ms Current timestamp in milliseconds.
 * @param active_state In-memory logical state updated at segment boundaries.
 * @return true if another step is needed, false if the player is done
 */
bool light_seq_step(light_player_t *player, int64_t now_ms, light_state_t *active_state);

/**
 * @brief Ask a running player to stop on its next step.
 *
 * @param player Player to stop.
 * @param stop_phase One of the LIGHT_SEQ_STOP_* phases.
 */
void light_seq_request_stop(light_player_t *player, light_seq_phase_t stop_phase);

/**
 * @brief Report whether a player is currently playing a sequence.
 *
 * @param player Player to test.
 * @return true if the player is not idle
 */
bool light_seq_is_active(const light_player_t *player);
