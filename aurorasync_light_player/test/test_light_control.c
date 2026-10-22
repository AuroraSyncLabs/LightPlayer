
/*
** test_light_control.c
** Unit tests for light control command validation
*/

#include <errno.h>
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

TEST_CASE("policy command rejects invalid values", "[light_control]")
{
    uint8_t command[] = {LIGHT_CMD_SET_POLICY, 255};
    TEST_ASSERT_EQUAL_INT(-EINVAL, light_control_validate_command(command, sizeof(command)));
}

TEST_CASE("commands reject incorrect lengths", "[light_control]")
{
    uint8_t master[] = {LIGHT_CMD_SET_MASTER, 1, 2};
    uint8_t global_color[] = {LIGHT_CMD_SET_GLOBAL_COLOR, 1, 2, 3};
    uint8_t rgb_strip[] = {LIGHT_CMD_SET_RGB_STRIP, 1, 2, 3};

    TEST_ASSERT_EQUAL_INT(-EMSGSIZE, light_control_validate_command(master, sizeof(master)));
    TEST_ASSERT_EQUAL_INT(-EMSGSIZE, light_control_validate_command(global_color, sizeof(global_color)));
    TEST_ASSERT_EQUAL_INT(-EMSGSIZE, light_control_validate_command(rgb_strip, sizeof(rgb_strip)));
}

TEST_CASE("unknown opcode is rejected", "[light_control]")
{
    uint8_t command[] = {0xFF, 0};
    TEST_ASSERT_EQUAL_INT(-EINVAL, light_control_validate_command(command, sizeof(command)));
}
