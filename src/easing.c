/*
** Julien ROIRON, 2026
** easing.c
** File description:
** Source file containing easing logic
*/

#include <stddef.h>

#include <lightplayer/easing.h>

const ease_curve_t EASE_LINEAR = EASE_LINEAR_INIT;
const ease_curve_t EASE_SMOOTH = EASE_SMOOTH_INIT;
const ease_curve_t EASE_RELEASE = EASE_RELEASE_INIT;

/** Number of cubic segments between the EASE_KNOTS knots. */
#define EASE_SEGMENTS (EASE_KNOTS - 1)

/** Width of one segment on the input axis, the knots are evenly spaced. */
#define EASE_SEGMENT_SPAN (EASE_SCALE / EASE_SEGMENTS)

/** Number of fractional bits used by the Q16 segment parameter. */
#define EASE_Q16_BITS 16

/**
 * @brief Function to build knots array.
 *
 * @param c The input curve settings
 * @param k The output knots
 */
static void ease_build_knots(const ease_curve_t *c, int32_t k[EASE_KNOTS])
{
    k[0] = 0;
    k[1] = c->points[0];
    k[2] = c->points[1];
    k[3] = c->points[2];
    k[4] = (int32_t)EASE_ONE;
}

/**
 * @brief Multiply a coefficient by the Q16 segment parameter.
 *
 * @param a The coefficient, expressed in output units
 * @param s The segment parameter in Q16, 65536 meaning the end of the segment
 * @return The product brought back to output units, rounded to the nearest value
 */
static int64_t ease_mul_q16(int64_t a, int64_t s)
{
    return (a * s + (1 << (EASE_Q16_BITS - 1))) >> EASE_Q16_BITS;
}

/**
 * @brief Compute the absolute value of a slope.
 *
 * @param v The input value
 * @return The absolute value of v
 */
static int32_t ease_abs(int32_t v)
{
    return v < 0 ? -v : v;
}

/**
 * @brief Compute the tangent of an interior knot.
 *
 * The weighted harmonic mean of the two neighbouring secants is the
 * Fritsch-Carlson tangent: it is null on a local extremum and never exceeds
 * three times the smallest secant, which is what keeps the curve monotone.
 *
 * @param prev The secant of the segment before the knot
 * @param next The secant of the segment after the knot
 * @return The tangent expressed in output units per segment
 */
static int32_t ease_tangent_interior(int32_t prev, int32_t next)
{
    /* A null or sign changing neighbour means the knot is a local extremum. */
    if ((prev <= 0 && next >= 0) || (prev >= 0 && next <= 0))
    {
        return 0;
    }
    return (int32_t)((2ll * prev * next) / ((int64_t)prev + (int64_t)next));
}

/**
 * @brief Compute the tangent of an endpoint knot.
 *
 * The one-sided three point formula is used, then limited so that the endpoint
 * never breaks the monotonicity of the first or last segment.
 *
 * @param near The secant of the segment touching the endpoint
 * @param far The secant of the next segment
 * @return The tangent expressed in output units per segment
 */
static int32_t ease_tangent_end(int32_t near, int32_t far)
{
    int32_t m;

    if (near == 0)
    {
        return 0;
    }
    m = (3 * near - far) / 2;
    /* Overshooting on the wrong side of the secant would break monotonicity. */
    if ((m ^ near) < 0)
    {
        return 0;
    }
    if ((near ^ far) < 0 && ease_abs(m) > 3 * ease_abs(near))
    {
        return 3 * near;
    }
    return m;
}

/**
 * @brief Function to build the secants and the tangents of the curve.
 *
 * @param k The knots of the curve
 * @param d The output secants, one per segment
 * @param m The output tangents, one per knot
 */
static void ease_build_tangents(const int32_t k[EASE_KNOTS], int32_t d[EASE_SEGMENTS],
                                int32_t m[EASE_KNOTS])
{
    for (int i = 0; i < EASE_SEGMENTS; i++)
    {
        d[i] = k[i + 1] - k[i];
    }
    m[0] = ease_tangent_end(d[0], d[1]);
    for (int i = 1; i < EASE_SEGMENTS; i++)
    {
        m[i] = ease_tangent_interior(d[i - 1], d[i]);
    }
    m[EASE_SEGMENTS] = ease_tangent_end(d[EASE_SEGMENTS - 1], d[EASE_SEGMENTS - 2]);
}

/**
 * @brief Evaluate one cubic Hermite segment.
 *
 * The polynomial is evaluated with Horner's scheme on the Q16 segment
 * parameter, so the whole curve stays in integer arithmetic.
 *
 * @param y The value of the knot starting the segment
 * @param d The secant of the segment
 * @param m0 The tangent of the knot starting the segment
 * @param m1 The tangent of the knot ending the segment
 * @param s The segment parameter in Q16, 0 at the start and 65536 at the end
 * @return The interpolated value, not clamped yet
 */
static int64_t ease_segment_value(int32_t y, int32_t d, int32_t m0, int32_t m1, int64_t s)
{
    /* The accumulator stays in Q16 so the three steps do not pile up rounding. */
    int64_t acc = (int64_t)(m0 + m1 - 2 * d) << EASE_Q16_BITS;

    acc = ((int64_t)(3 * d - 2 * m0 - m1) << EASE_Q16_BITS) + ease_mul_q16(acc, s);
    acc = ((int64_t)m0 << EASE_Q16_BITS) + ease_mul_q16(acc, s);
    acc = ((int64_t)y << EASE_Q16_BITS) + ease_mul_q16(acc, s);
    return (acc + (1 << (EASE_Q16_BITS - 1))) >> EASE_Q16_BITS;
}

uint16_t ease_apply_points(const uint16_t points[EASE_POINTS], uint32_t t)
{
    ease_curve_t curve;

    if (points == NULL)
    {
        return 0;
    }
    for (int i = 0; i < EASE_POINTS; i++)
    {
        curve.points[i] = points[i];
    }
    return ease_apply(&curve, t);
}

uint16_t ease_apply(const ease_curve_t *c, uint32_t t)
{
    int32_t k[EASE_KNOTS];
    int32_t d[EASE_SEGMENTS];
    int32_t m[EASE_KNOTS];
    uint32_t segment;
    int64_t s;
    int64_t value;

    if (c == NULL)
    {
        return 0;
    }
    ease_build_knots(c, k);
    if (t >= EASE_SCALE)
    {
        return (uint16_t)k[EASE_KNOTS - 1];
    }
    ease_build_tangents(k, d, m);
    segment = t / EASE_SEGMENT_SPAN;
    s = ((int64_t)(t % EASE_SEGMENT_SPAN) << EASE_Q16_BITS) / EASE_SEGMENT_SPAN;
    value = ease_segment_value(k[segment], d[segment], m[segment], m[segment + 1], s);
    if (value < 0)
    {
        return 0;
    }
    if (value > (int64_t)EASE_ONE)
    {
        return EASE_ONE;
    }
    return (uint16_t)value;
}
