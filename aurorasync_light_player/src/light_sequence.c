/*
** Julien ROIRON, 2026
** light_sequence.c
** File description:
** Source file implementing the light sequence player
*/

#include <errno.h>
#include <inttypes.h>
#include <string.h>

#include <esp_log.h>

#include <lightplayer/led_pwm.h>
#include <lightplayer/light_sequence.h>

static const char *TAG = "LightPlayer_LIGHT_SEQUENCE";

#define LOG_INF(...) ESP_LOGI(TAG, __VA_ARGS__)
#define LOG_ERR(...) ESP_LOGE(TAG, __VA_ARGS__)
#define LOG_WRN(...) ESP_LOGW(TAG, __VA_ARGS__)

/**
 * Upper bound on the number of segments a single step may skip over. A step
 * arriving very late still catches up, but a pathological sequence can never
 * spin the caller inside this module.
 */
#define LIGHT_SEQ_MAX_CATCH_UP 64

void light_seq_clear(light_player_t *player)
{
    memset(player, 0, sizeof(light_player_t));
    player->phase = LIGHT_SEQ_IDLE;
}

bool light_seq_is_active(const light_player_t *player)
{
    return player->phase != LIGHT_SEQ_IDLE;
}

void light_seq_request_stop(light_player_t *player, light_seq_phase_t stop_phase)
{
    if (player->phase == LIGHT_SEQ_IDLE)
    {
        return;
    }
    if (stop_phase != LIGHT_SEQ_STOP_KEEP_CURRENT && stop_phase != LIGHT_SEQ_STOP_ROLLBACK_START &&
        stop_phase != LIGHT_SEQ_STOP_TO_TARGET)
    {
        LOG_ERR("Invalid stop phase requested (err: %d)", stop_phase);
        return;
    }
    player->phase = stop_phase;
}

/**
 * @brief Return the segment the player is currently on.
 *
 * @param player Player to read
 * @return The current segment
 */
static const light_segment_t *light_seq_current(const light_player_t *player)
{
    return &player->sequence.segments[player->current_segment];
}

/**
 * @brief Return the state the sequence rests on once every loop is done.
 *
 * @param player Player to read
 * @return The target state of the last segment
 */
static const light_state_t *light_seq_final_state(const light_player_t *player)
{
    return &player->sequence.segments[player->sequence.num_segments - 1].to;
}

/**
 * @brief Render the endpoints of the current segment into the cached frames.
 *
 * @param player Player whose current segment endpoints must be rendered
 */
static void light_seq_render_segment(light_player_t *player)
{
    int err;

    err = light_control_render(&player->segment_origin, &player->origin_frame);
    if (err != 0)
    {
        LOG_ERR("Failed to render segment origin frame (err: %d)", err);
    }
    err = light_control_render(&light_seq_current(player)->to, &player->target_frame);
    if (err != 0)
    {
        LOG_ERR("Failed to render segment target frame (err: %d)", err);
    }
}

/**
 * @brief Apply a light state immediately and store it as the current frame.
 *
 * @param player Player to update
 * @param state Light state to apply
 * @param active_state In-memory logical state to update
 */
static void light_seq_settle_on(light_player_t *player, const light_state_t *state,
                                light_state_t *active_state)
{
    int err;

    err = light_control_render(state, &player->current_frame);
    if (err != 0)
    {
        LOG_ERR("Failed to render settle frame (err: %d)", err);
        return;
    }
    *active_state = *state;
    err = light_control_apply_frame(&player->current_frame);
    if (err != 0)
    {
        LOG_ERR("Failed to apply settle frame (err: %d)", err);
    }
}

/**
 * @brief Report whether the sequence has at least one segment lasting some time.
 *
 * @param sequence Sequence to check
 * @return true if the sequence can make progress over time
 */
static bool light_seq_has_duration(const light_seq_t *sequence)
{
    for (uint8_t i = 0; i < sequence->num_segments; i++)
    {
        if (sequence->segments[i].duration_ms > 0)
        {
            return true;
        }
    }
    return false;
}

int light_seq_start(light_player_t *player, const light_seq_t *sequence,
                    const light_state_t *current, int64_t now_ms)
{
    if (player == NULL || sequence == NULL || current == NULL)
    {
        return -EINVAL;
    }
    if (sequence->num_segments == 0 || sequence->num_segments > LIGHT_SEQ_MAX_SEGMENTS)
    {
        LOG_ERR("Rejecting sequence with %u segments", sequence->num_segments);
        light_seq_clear(player);
        return -EINVAL;
    }
    /* A sequence with no duration at all still settles on its last segment,
       which is a legitimate instant transition. Only an endless one has to be
       rejected: it would never make progress and never end. */
    if (sequence->loops == 0 && !light_seq_has_duration(sequence))
    {
        LOG_ERR("Rejecting an endless sequence with a null total duration");
        light_seq_clear(player);
        return -EINVAL;
    }
    light_seq_clear(player);
    player->sequence = *sequence;
    player->sequence.origin = *current;
    player->segment_origin = *current;
    player->phase = LIGHT_SEQ_RUNNING;
    player->segment_start_ms = now_ms;
    light_seq_render_segment(player);
    player->current_frame = player->origin_frame;
    LOG_INF("Sequence started with %u segments, loops=%" PRIu32, sequence->num_segments,
            sequence->loops);
    return 0;
}

/**
 * @brief Finish the sequence and rest on the state of its last segment.
 *
 * @param player Player to finish
 * @param active_state In-memory logical state to update
 * @return false, the player has no more step to run
 */
static bool light_seq_finish(light_player_t *player, light_state_t *active_state)
{
    light_seq_settle_on(player, light_seq_final_state(player), active_state);
    LOG_INF("Sequence complete");
    light_seq_clear(player);
    return false;
}

/**
 * @brief Commit the current segment and move the player to the next one.
 *
 * The segment target becomes the origin of the next segment, so a sequence
 * always chains from the state it actually reached.
 *
 * @param player Player to advance
 * @param active_state In-memory logical state to update
 * @return true if a next segment must be played, false if the sequence is over
 */
static bool light_seq_advance(light_player_t *player, light_state_t *active_state)
{
    const light_segment_t *segment = light_seq_current(player);

    player->segment_start_ms += segment->duration_ms;
    player->segment_origin = segment->to;
    *active_state = segment->to;
    player->current_segment++;
    if (player->current_segment < player->sequence.num_segments)
    {
        light_seq_render_segment(player);
        return true;
    }
    player->current_segment = 0;
    player->loop_done++;
    if (player->sequence.loops != 0 && player->loop_done >= player->sequence.loops)
    {
        return false;
    }
    light_seq_render_segment(player);
    return true;
}

/**
 * @brief Compute the position inside the current segment.
 *
 * @param player Player to read
 * @param now_ms Current timestamp in milliseconds
 * @return The position, from 0 to EASE_SCALE
 */
static uint32_t light_seq_progress(const light_player_t *player, int64_t now_ms)
{
    int64_t duration = light_seq_current(player)->duration_ms;
    int64_t elapsed = now_ms - player->segment_start_ms;

    if (duration <= 0 || elapsed >= duration)
    {
        return EASE_SCALE;
    }
    if (elapsed <= 0)
    {
        return 0;
    }
    return (uint32_t)((elapsed * EASE_SCALE) / duration);
}

/**
 * @brief Interpolate one PWM channel between the two segment endpoints.
 *
 * @param from Level at the start of the segment
 * @param to Level at the end of the segment
 * @param eased Eased position, from 0 to EASE_ONE
 * @return The interpolated level
 */
static led_pwm_level_t light_seq_lerp_level(led_pwm_level_t from, led_pwm_level_t to,
                                            uint16_t eased)
{
    int32_t delta = (int32_t)to - (int32_t)from;

    return (led_pwm_level_t)(from + (int32_t)(((int64_t)delta * eased) / EASE_ONE));
}

/**
 * @brief Build the frame of the current segment at the given eased position.
 *
 * @param player Player holding the cached endpoint frames
 * @param eased Eased position, from 0 to EASE_ONE
 */
static void light_seq_interpolate(light_player_t *player, uint16_t eased)
{
    for (int i = 0; i < LED_PWM_CHANNEL_COUNT; i++)
    {
        player->current_frame.levels[i] = light_seq_lerp_level(
            player->origin_frame.levels[i], player->target_frame.levels[i], eased);
    }
}

/**
 * @brief Handle a stop request and clear the player.
 *
 * @param player Player to stop
 * @param active_state In-memory logical state to update
 */
static void light_seq_handle_stop(light_player_t *player, light_state_t *active_state)
{
    int err;

    switch (player->phase)
    {
    case LIGHT_SEQ_STOP_KEEP_CURRENT:
        err = light_control_apply_frame(&player->current_frame);
        if (err != 0)
        {
            LOG_ERR("Failed to apply current sequence frame (err: %d)", err);
        }
        LOG_INF("Sequence stopped at current frame");
        break;
    case LIGHT_SEQ_STOP_ROLLBACK_START:
        light_seq_settle_on(player, &player->sequence.origin, active_state);
        LOG_INF("Sequence stopped and rolled back");
        break;
    case LIGHT_SEQ_STOP_TO_TARGET:
        light_seq_settle_on(player, light_seq_final_state(player), active_state);
        LOG_INF("Sequence stopped at target");
        break;
    default:
        LOG_ERR("Unexpected sequence phase in stop handler (err: %d)", player->phase);
        break;
    }
    light_seq_clear(player);
}

/**
 * @brief Skip every segment already fully elapsed at the given timestamp.
 *
 * @param player Player to advance
 * @param now_ms Current timestamp in milliseconds
 * @param active_state In-memory logical state to update
 * @param done Set to true when the sequence ended while catching up
 * @return true if the player is still running
 */
static bool light_seq_catch_up(light_player_t *player, int64_t now_ms, light_state_t *active_state,
                               bool *done)
{
    for (int i = 0; i < LIGHT_SEQ_MAX_CATCH_UP; i++)
    {
        if (light_seq_progress(player, now_ms) != EASE_SCALE)
        {
            return true;
        }
        if (!light_seq_advance(player, active_state))
        {
            *done = true;
            return false;
        }
    }
    /* The player fell too far behind to be resynchronized segment by segment. */
    LOG_WRN("Sequence resynchronized after %d skipped segments", LIGHT_SEQ_MAX_CATCH_UP);
    player->segment_start_ms = now_ms;
    return true;
}

bool light_seq_step(light_player_t *player, int64_t now_ms, light_state_t *active_state)
{
    bool done = false;
    uint16_t eased;
    int err;

    if (player == NULL || active_state == NULL)
    {
        return false;
    }
    if (player->phase == LIGHT_SEQ_IDLE)
    {
        return false;
    }
    if (player->phase != LIGHT_SEQ_RUNNING)
    {
        light_seq_handle_stop(player, active_state);
        return false;
    }
    if (!light_seq_catch_up(player, now_ms, active_state, &done))
    {
        return done ? light_seq_finish(player, active_state) : false;
    }
    eased = ease_apply(&light_seq_current(player)->curve, light_seq_progress(player, now_ms));
    light_seq_interpolate(player, eased);
    err = light_control_apply_frame(&player->current_frame);
    if (err != 0)
    {
        LOG_ERR("Failed to apply sequence frame (err: %d)", err);
        light_seq_clear(player);
        return false;
    }
    return true;
}
