
/*
** test_light_control.c
** Unit tests for light control command validation
*/

#include <errno.h>
#include <stdint.h>
#include <unity.h>
#include <lightplayer/light_control.h>

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
    TEST_ASSERT_EQUAL_INT(-EMSGSIZE, light_control_validate_command(global_color, sizeof(global_color)));
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
