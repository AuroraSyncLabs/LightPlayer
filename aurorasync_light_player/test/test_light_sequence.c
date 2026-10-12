/*
** Julien ROIRON, 2026
** test_light_sequence.c
** File description:
** Unit tests of the light sequence player
*/

#include <errno.h>
#include <unity.h>

#include <lightplayer/led_pwm.h>
#include <lightplayer/light_sequence.h>

static light_player_t player;
static light_state_t off;
static light_state_t red;
static light_state_t active;

void setUp(void)
{
    static bool pwm_ready = false;

    if (!pwm_ready)
    {
        TEST_ASSERT_EQUAL_INT(0, led_pwm_init());
        pwm_ready = true;
    }
    light_seq_clear(&player);
    light_control_init(&off);
    light_control_init(&red);
    light_control_set_master(&red, true);
    light_control_set_global(&red, 255, 255, 0, 0);
    active = off;
}

void tearDown(void)
{
}

/** Build a one-shot sequence going from the current state to red. */
static light_seq_t one_segment_to_red(uint32_t duration_ms)
{
    light_seq_t seq = {0};

    seq.loops = 1;
    seq.num_segments = 1;
    seq.segments[0].to = red;
    seq.segments[0].duration_ms = duration_ms;
    seq.segments[0].curve = EASE_LINEAR;
    return seq;
}

TEST_CASE("start rejects NULL arguments", "[light_sequence]")
{
    light_seq_t seq = one_segment_to_red(1000);

    TEST_ASSERT_EQUAL_INT(-EINVAL, light_seq_start(NULL, &seq, &off, 0));
    TEST_ASSERT_EQUAL_INT(-EINVAL, light_seq_start(&player, NULL, &off, 0));
    TEST_ASSERT_EQUAL_INT(-EINVAL, light_seq_start(&player, &seq, NULL, 0));
}

TEST_CASE("start rejects an invalid segment count", "[light_sequence]")
{
    light_seq_t seq = one_segment_to_red(1000);

    seq.num_segments = 0;
    TEST_ASSERT_EQUAL_INT(-EINVAL, light_seq_start(&player, &seq, &off, 0));
    seq.num_segments = LIGHT_SEQ_MAX_SEGMENTS + 1;
    TEST_ASSERT_EQUAL_INT(-EINVAL, light_seq_start(&player, &seq, &off, 0));
    TEST_ASSERT_FALSE(light_seq_is_active(&player));
}

TEST_CASE("start rejects an endless sequence without duration", "[light_sequence]")
{
    light_seq_t seq = one_segment_to_red(0);

    seq.loops = 0;
    TEST_ASSERT_EQUAL_INT(-EINVAL, light_seq_start(&player, &seq, &off, 0));
    TEST_ASSERT_FALSE(light_seq_is_active(&player));
}

TEST_CASE("one-shot sequence ends on its target", "[light_sequence]")
{
    light_seq_t seq = one_segment_to_red(1000);

    TEST_ASSERT_EQUAL_INT(0, light_seq_start(&player, &seq, &off, 0));
    TEST_ASSERT_TRUE(light_seq_step(&player, 0, &active));
    TEST_ASSERT_FALSE(light_seq_step(&player, 1000, &active));
    TEST_ASSERT_FALSE(light_seq_is_active(&player));
    TEST_ASSERT_TRUE(active.master_on);
    TEST_ASSERT_EQUAL_UINT8(255, active.global.r);
}

TEST_CASE("linear segment is halfway at mid duration", "[light_sequence]")
{
    light_seq_t seq = one_segment_to_red(1000);
    light_output_frame_t target;

    TEST_ASSERT_EQUAL_INT(0, light_control_render(&red, &target));
    light_seq_start(&player, &seq, &off, 0);
    TEST_ASSERT_TRUE(light_seq_step(&player, 500, &active));
    TEST_ASSERT_UINT16_WITHIN(1, target.levels[LED_PWM_CHANNEL_RGB_RED] / 2,
                              player.current_frame.levels[LED_PWM_CHANNEL_RGB_RED]);
    /* The logical state only changes on segment boundaries. */
    TEST_ASSERT_FALSE(active.master_on);
}

TEST_CASE("rollback stop restores the origin", "[light_sequence]")
{
    light_seq_t seq = one_segment_to_red(1000);

    light_seq_start(&player, &seq, &off, 0);
    light_seq_step(&player, 500, &active);
    light_seq_request_stop(&player, LIGHT_SEQ_STOP_ROLLBACK_START);
    TEST_ASSERT_FALSE(light_seq_step(&player, 600, &active));
    TEST_ASSERT_FALSE(light_seq_is_active(&player));
    TEST_ASSERT_FALSE(active.master_on);
}

TEST_CASE("endless sequence keeps running", "[light_sequence]")
{
    light_seq_t seq = one_segment_to_red(100);

    seq.loops = 0;
    light_seq_start(&player, &seq, &off, 0);
    TEST_ASSERT_TRUE(light_seq_step(&player, 100000, &active));
    TEST_ASSERT_TRUE(light_seq_is_active(&player));
}
