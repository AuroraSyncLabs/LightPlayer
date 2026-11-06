/*
** Julien ROIRON, 2026
** test_easing.c
** File description:
** Unit tests of the easing curves
*/

#include <stddef.h>
#include <stdint.h>
#include <unity.h>

#include <lightplayer/easing.h>

static const ease_curve_t *const curves[] = {&EASE_LINEAR, &EASE_SMOOTH, &EASE_RELEASE};

#define CURVE_COUNT (sizeof(curves) / sizeof(curves[0]))

/** Input distance between two knots, they are evenly spaced. */
#define KNOT_SPAN (EASE_SCALE / (EASE_KNOTS - 1))

TEST_CASE("NULL curve evaluates to 0", "[easing]")
{
    TEST_ASSERT_EQUAL_UINT16(0, ease_apply(NULL, EASE_SCALE / 2));
    TEST_ASSERT_EQUAL_UINT16(0, ease_apply_points(NULL, EASE_SCALE / 2));
}

TEST_CASE("curves start at 0", "[easing]")
{
    for (size_t i = 0; i < CURVE_COUNT; i++)
    {
        TEST_ASSERT_EQUAL_UINT16(0, ease_apply(curves[i], 0));
    }
}

TEST_CASE("curves reach EASE_ONE from EASE_SCALE on", "[easing]")
{
    for (size_t i = 0; i < CURVE_COUNT; i++)
    {
        TEST_ASSERT_EQUAL_UINT16(EASE_ONE, ease_apply(curves[i], EASE_SCALE));
        TEST_ASSERT_EQUAL_UINT16(EASE_ONE, ease_apply(curves[i], EASE_SCALE + 1));
        TEST_ASSERT_EQUAL_UINT16(EASE_ONE, ease_apply(curves[i], UINT32_MAX));
    }
}

TEST_CASE("curves go through their knots", "[easing]")
{
    for (size_t i = 0; i < CURVE_COUNT; i++)
    {
        for (uint32_t k = 0; k < EASE_POINTS; k++)
        {
            TEST_ASSERT_EQUAL_UINT16(curves[i]->points[k],
                                     ease_apply(curves[i], (k + 1) * KNOT_SPAN));
        }
    }
}

TEST_CASE("curves never decrease", "[easing]")
{
    for (size_t i = 0; i < CURVE_COUNT; i++)
    {
        uint16_t previous = 0;

        for (uint32_t t = 0; t <= EASE_SCALE; t++)
        {
            uint16_t value = ease_apply(curves[i], t);

            TEST_ASSERT_GREATER_OR_EQUAL_UINT16(previous, value);
            previous = value;
        }
    }
}
