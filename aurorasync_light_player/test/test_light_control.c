/*
** test_light_control.c
** File description:
** Unit tests for light control command validation
*/

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <unity.h>

#include <lightplayer/light_control.h>

/** Return a state from light_control_init(): global mode, every color at 255. */
static light_state_t init_state(void)
{
    light_state_t state;

    TEST_ASSERT_EQUAL_INT(0, light_control_init(&state));
    return state;
}

static uint8_t encoded_flags(const light_state_t *state)
{
    uint8_t payload[LIGHT_STATE_PAYLOAD_SIZE];

    TEST_ASSERT_EQUAL_INT(0, light_control_encode_state(state, payload));
    return payload[1];
}

static void assert_same_mode_and_colors(const light_state_t *expected, const light_state_t *actual)
{
    TEST_ASSERT_EQUAL(expected->global_mode, actual->global_mode);
    TEST_ASSERT_EQUAL_MEMORY(&expected->global, &actual->global, sizeof(actual->global));
    TEST_ASSERT_EQUAL_MEMORY(&expected->rgb_strip, &actual->rgb_strip, sizeof(actual->rgb_strip));
    TEST_ASSERT_EQUAL_UINT8(expected->flamewarm_brightness, actual->flamewarm_brightness);
    TEST_ASSERT_EQUAL_UINT8(expected->skyblue_brightness, actual->skyblue_brightness);
}

TEST_CASE("empty command is rejected", "[light_control]")
{
    TEST_ASSERT_EQUAL_INT(-EMSGSIZE, light_control_validate_command(NULL, 0));
}

TEST_CASE("master command accepts valid values", "[light_control]")
{
    uint8_t command[] = {LIGHT_CMD_SET_MASTER, 0};

    TEST_ASSERT_EQUAL_INT(0, light_control_validate_command(command, sizeof(command)));
    command[1] = 1;
    TEST_ASSERT_EQUAL_INT(0, light_control_validate_command(command, sizeof(command)));
}

TEST_CASE("master command rejects invalid values", "[light_control]")
{
    uint8_t command[] = {LIGHT_CMD_SET_MASTER, 2};

    TEST_ASSERT_EQUAL_INT(-EINVAL, light_control_validate_command(command, sizeof(command)));
}

TEST_CASE("policy command accepts valid values", "[light_control]")
{
    uint8_t command[] = {LIGHT_CMD_SET_POLICY, 0};

    TEST_ASSERT_EQUAL_INT(0, light_control_validate_command(command, sizeof(command)));
    command[1] = LIGHT_MIX_POLICY_MAX_BRIGHTNESS;
    TEST_ASSERT_EQUAL_INT(0, light_control_validate_command(command, sizeof(command)));
}

TEST_CASE("policy command rejects invalid values", "[light_control]")
{
    uint8_t command[] = {LIGHT_CMD_SET_POLICY, LIGHT_MIX_POLICY_MAX_BRIGHTNESS + 1};

    TEST_ASSERT_EQUAL_INT(-EINVAL, light_control_validate_command(command, sizeof(command)));
}

TEST_CASE("commands reject incorrect lengths", "[light_control]")
{
    uint8_t master[] = {LIGHT_CMD_SET_MASTER, 1, 2};
    uint8_t global_color[] = {LIGHT_CMD_SET_GLOBAL_COLOR, 1, 2};
    uint8_t rgb_strip[] = {LIGHT_CMD_SET_RGB_STRIP, 1, 2, 3};
    uint8_t global[] = {LIGHT_CMD_SET_GLOBAL, 1, 2, 3, 4, 5};

    TEST_ASSERT_EQUAL_INT(-EMSGSIZE, light_control_validate_command(master, sizeof(master)));
    TEST_ASSERT_EQUAL_INT(-EMSGSIZE,
                          light_control_validate_command(global_color, sizeof(global_color)));
    TEST_ASSERT_EQUAL_INT(-EMSGSIZE, light_control_validate_command(rgb_strip, sizeof(rgb_strip)));
    TEST_ASSERT_EQUAL_INT(-EMSGSIZE, light_control_validate_command(global, sizeof(global)));
}

TEST_CASE("unknown opcode is rejected", "[light_control]")
{
    uint8_t command[] = {0xFF, 0};

    TEST_ASSERT_EQUAL_INT(-EINVAL, light_control_validate_command(command, sizeof(command)));
}

TEST_CASE("commands with correct lengths are accepted", "[light_control]")
{
    uint8_t brightness[] = {LIGHT_CMD_SET_GLOBAL_BRIGHTNESS, 50};
    uint8_t flamewarm[] = {LIGHT_CMD_SET_FLAMEWARM, 50};
    uint8_t skyblue[] = {LIGHT_CMD_SET_SKYBLUE, 50};
    uint8_t global_color[] = {LIGHT_CMD_SET_GLOBAL_COLOR, 1, 2, 3};
    uint8_t rgb_strip[] = {LIGHT_CMD_SET_RGB_STRIP, 1, 2, 3, 4};
    uint8_t global[] = {LIGHT_CMD_SET_GLOBAL, 1, 2, 3, 4};

    TEST_ASSERT_EQUAL_INT(0, light_control_validate_command(brightness, sizeof(brightness)));
    TEST_ASSERT_EQUAL_INT(0, light_control_validate_command(flamewarm, sizeof(flamewarm)));
    TEST_ASSERT_EQUAL_INT(0, light_control_validate_command(skyblue, sizeof(skyblue)));
    TEST_ASSERT_EQUAL_INT(0, light_control_validate_command(global_color, sizeof(global_color)));
    TEST_ASSERT_EQUAL_INT(0, light_control_validate_command(rgb_strip, sizeof(rgb_strip)));
    TEST_ASSERT_EQUAL_INT(0, light_control_validate_command(global, sizeof(global)));
}

TEST_CASE("master command switches the master", "[light_control]")
{
    const uint8_t on[] = {LIGHT_CMD_SET_MASTER, 1};
    const uint8_t off[] = {LIGHT_CMD_SET_MASTER, 0};
    light_state_t state = init_state();

    light_control_apply_command(&state, on);
    TEST_ASSERT_TRUE(state.master_on);
    light_control_apply_command(&state, off);
    TEST_ASSERT_FALSE(state.master_on);
}

TEST_CASE("global command sets the global brightness and color", "[light_control]")
{
    const uint8_t command[] = {LIGHT_CMD_SET_GLOBAL, 10, 20, 30, 40};
    const light_rgb_state_t expected = {10, 20, 30, 40};
    light_state_t state = init_state();

    light_control_apply_command(&state, command);
    TEST_ASSERT_EQUAL_MEMORY(&expected, &state.global, sizeof(expected));
}

TEST_CASE("global brightness command only sets the global brightness", "[light_control]")
{
    const uint8_t command[] = {LIGHT_CMD_SET_GLOBAL_BRIGHTNESS, 10};
    const light_rgb_state_t expected = {10, 255, 255, 255};
    light_state_t state = init_state();

    light_control_apply_command(&state, command);
    TEST_ASSERT_EQUAL_MEMORY(&expected, &state.global, sizeof(expected));
}

TEST_CASE("global color command only sets the global color", "[light_control]")
{
    const uint8_t command[] = {LIGHT_CMD_SET_GLOBAL_COLOR, 20, 30, 40};
    const light_rgb_state_t expected = {255, 20, 30, 40};
    light_state_t state = init_state();

    light_control_apply_command(&state, command);
    TEST_ASSERT_EQUAL_MEMORY(&expected, &state.global, sizeof(expected));
}

TEST_CASE("RGB strip command sets the RGB strip", "[light_control]")
{
    const uint8_t command[] = {LIGHT_CMD_SET_RGB_STRIP, 10, 20, 30, 40};
    const light_rgb_state_t expected = {10, 20, 30, 40};
    light_state_t state = init_state();

    light_control_apply_command(&state, command);
    TEST_ASSERT_EQUAL_MEMORY(&expected, &state.rgb_strip, sizeof(expected));
}

TEST_CASE("FlameWarm command sets the FlameWarm brightness", "[light_control]")
{
    const uint8_t command[] = {LIGHT_CMD_SET_FLAMEWARM, 10};
    light_state_t state = init_state();

    light_control_apply_command(&state, command);
    TEST_ASSERT_EQUAL_UINT8(10, state.flamewarm_brightness);
}

TEST_CASE("SkyBlue command sets the SkyBlue brightness", "[light_control]")
{
    const uint8_t command[] = {LIGHT_CMD_SET_SKYBLUE, 10};
    light_state_t state = init_state();

    light_control_apply_command(&state, command);
    TEST_ASSERT_EQUAL_UINT8(10, state.skyblue_brightness);
}

TEST_CASE("policy command sets the mix policy", "[light_control]")
{
    const uint8_t max_brightness[] = {LIGHT_CMD_SET_POLICY, LIGHT_MIX_POLICY_MAX_BRIGHTNESS};
    const uint8_t color_accurate[] = {LIGHT_CMD_SET_POLICY, LIGHT_MIX_POLICY_COLOR_ACCURATE};
    light_state_t state = init_state();

    light_control_apply_command(&state, max_brightness);
    TEST_ASSERT_EQUAL(LIGHT_MIX_POLICY_MAX_BRIGHTNESS, state.mix_policy);
    light_control_apply_command(&state, color_accurate);
    TEST_ASSERT_EQUAL(LIGHT_MIX_POLICY_COLOR_ACCURATE, state.mix_policy);
}

TEST_CASE("per-strip commands leave global mode, global commands restore it", "[light_control]")
{
    const uint8_t per_strip[][5] = {
        {LIGHT_CMD_SET_RGB_STRIP, 1, 2, 3, 4},
        {LIGHT_CMD_SET_FLAMEWARM, 1},
        {LIGHT_CMD_SET_SKYBLUE, 1},
    };
    const uint8_t global[][5] = {
        {LIGHT_CMD_SET_GLOBAL, 1, 2, 3, 4},
        {LIGHT_CMD_SET_GLOBAL_BRIGHTNESS, 1},
        {LIGHT_CMD_SET_GLOBAL_COLOR, 1, 2, 3},
    };
    light_state_t state = init_state();

    for (size_t i = 0; i < sizeof(per_strip) / sizeof(per_strip[0]); i++)
    {
        light_control_apply_command(&state, per_strip[i]);
        TEST_ASSERT_FALSE(state.global_mode);
        light_control_apply_command(&state, global[i]);
        TEST_ASSERT_TRUE(state.global_mode);
    }
}

TEST_CASE("master and policy commands keep the mode and the colors", "[light_control]")
{
    const uint8_t master[] = {LIGHT_CMD_SET_MASTER, 1};
    const uint8_t policy[] = {LIGHT_CMD_SET_POLICY, LIGHT_MIX_POLICY_MAX_BRIGHTNESS};
    const bool modes[] = {true, false};
    light_state_t state = init_state();
    light_state_t before;

    for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); i++)
    {
        state.global_mode = modes[i];
        before = state;
        light_control_apply_command(&state, master);
        light_control_apply_command(&state, policy);
        assert_same_mode_and_colors(&before, &state);
    }
}

TEST_CASE("encode state rejects NULL arguments", "[light_control]")
{
    light_state_t state = init_state();
    uint8_t payload[LIGHT_STATE_PAYLOAD_SIZE];

    TEST_ASSERT_EQUAL_INT(-EINVAL, light_control_encode_state(NULL, payload));
    TEST_ASSERT_EQUAL_INT(-EINVAL, light_control_encode_state(&state, NULL));
}

TEST_CASE("initial state encodes master off, global mode and full white", "[light_control]")
{
    const uint8_t expected[LIGHT_STATE_PAYLOAD_SIZE] = {1,   0x02, 255, 255, 255, 255,
                                                        255, 255,  255, 255, 255, 255};
    light_state_t state = init_state();
    uint8_t payload[LIGHT_STATE_PAYLOAD_SIZE];

    TEST_ASSERT_EQUAL_INT(0, light_control_encode_state(&state, payload));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, payload, LIGHT_STATE_PAYLOAD_SIZE);
}

TEST_CASE("each flag sets its own bit", "[light_control]")
{
    TEST_ASSERT_EQUAL_HEX8(0x00, encoded_flags(&(light_state_t){0}));
    TEST_ASSERT_EQUAL_HEX8(0x01, encoded_flags(&(light_state_t){.master_on = true}));
    TEST_ASSERT_EQUAL_HEX8(0x02, encoded_flags(&(light_state_t){.global_mode = true}));
    TEST_ASSERT_EQUAL_HEX8(
        0x04, encoded_flags(&(light_state_t){.mix_policy = LIGHT_MIX_POLICY_MAX_BRIGHTNESS}));
}

TEST_CASE("each field is encoded at its offset", "[light_control]")
{
    /* Each field holds its own offset, so a swap shows up. */
    const light_state_t state = {
        .global = {2, 3, 4, 5},
        .rgb_strip = {6, 7, 8, 9},
        .flamewarm_brightness = 10,
        .skyblue_brightness = 11,
    };
    const uint8_t expected[LIGHT_STATE_PAYLOAD_SIZE] = {1, 0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    uint8_t payload[LIGHT_STATE_PAYLOAD_SIZE];

    TEST_ASSERT_EQUAL_INT(0, light_control_encode_state(&state, payload));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, payload, LIGHT_STATE_PAYLOAD_SIZE);
}
