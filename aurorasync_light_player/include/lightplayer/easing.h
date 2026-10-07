/*
** Julien ROIRON, 2026
** easing.h
** File description:
** Include file containing the easing functions
*/

#pragma once

#include <stdint.h>

#define EASE_SCALE 65536
#define EASE_KNOTS (EASE_POINTS + 2) /* Endpoints are implicit 0 and 65535 */
#define EASE_POINTS 3
#define EASE_ONE 65535u

/**
 * @brief Structure representing easing.
 *
 * @var points Represents the easing value at 25/50/75%
 */
typedef struct ease_curve_s
{
    uint16_t points[EASE_POINTS];
} ease_curve_t;

/**
 * Brace initializers for the three stock curves.
 *
 * A const object is not a constant expression in C, so the EASE_* objects below
 * cannot initialize another object with static storage duration. Tables of
 * curves defined at file scope must use these macros instead.
 */
#define EASE_LINEAR_INIT {{16384, 32768, 49152}}
#define EASE_SMOOTH_INIT {{9088, 32768, 56448}}
#define EASE_RELEASE_INIT {{25080, 46341, 60547}}

extern const ease_curve_t EASE_LINEAR;
extern const ease_curve_t EASE_SMOOTH;
extern const ease_curve_t EASE_RELEASE;

/**
 * @brief Evaluate an easing curve with a monotone cubic Hermite interpolation.
 *
 * The curve goes through the five knots 0, points[0], points[1], points[2] and
 * EASE_ONE, evenly spaced at 0/25/50/75/100% of the input axis. The tangents
 * come from the Fritsch-Carlson filter, so the result never overshoots nor
 * wiggles between two knots.
 *
 * @param c The curve to evaluate
 * @param t The position on the curve, from 0 to EASE_SCALE
 * @return The eased value, from 0 to EASE_ONE, 0 when c is NULL
 */
uint16_t ease_apply(const ease_curve_t *c, uint32_t t);

/**
 * @brief Evaluate an easing curve stored as a raw array of control points.
 *
 * @param points The EASE_POINTS control points of the curve
 * @param t The position on the curve, from 0 to EASE_SCALE
 * @return The eased value, from 0 to EASE_ONE, 0 when points is NULL
 */
uint16_t ease_apply_points(const uint16_t points[EASE_POINTS], uint32_t t);
