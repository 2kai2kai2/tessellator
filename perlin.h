#pragma once
// Based on implementation in
// https://en.wikipedia.org/wiki/Perlin_noise#Implementation

#include <cmath>
#include <ctime>
#include <random>

struct PerlinGen {
    std::default_random_engine engine;
    unsigned rand_a, rand_b, rand_c;

    PerlinGen()
        : engine(time(NULL)), rand_a(engine()), rand_b(engine()),
          rand_c(engine()) {}

    /* Function to linearly interpolate between a0 and a1
     * Weight w should be in the range [0.0, 1.0]
     */
    static double interpolate(double a0, double a1, double w) {
        /* // You may want clamping by inserting:
         * if (0.0 > w) return a0;
         * if (1.0 < w) return a1;
         */
        // return (a1 - a0) * w + a0;
        // Use this cubic interpolation [[Smoothstep]] instead, for a smooth
        // appearance:
        return (a1 - a0) * (3.0 - w * 2.0) * w * w + a0;
        /*
         * // Use [[Smootherstep]] for an even smoother result with a second
         * derivative equal to zero on boundaries: return (a1 - a0) * ((w * (w
         * * 6.0
         * - 15.0) + 10.0) * w * w * w) + a0;
         */
    }

    typedef struct {
        double x, y;
    } vector2;

    /* Create pseudorandom direction vector
     */
    vector2 randomGradient(int ix, int iy) const {
        // No precomputed gradients mean this works for any number of grid
        // coordinates
        const unsigned w = 8 * sizeof(unsigned);
        const unsigned s = w / 2; // rotation width
        unsigned a = ix, b = iy;
        a *= rand_a;
        b ^= a << s | a >> (w - s);
        b *= rand_b;
        a ^= b << s | b >> (w - s);
        a *= rand_c;
        float random = a * (3.14159265 / ~(~0u >> 1)); // in [0, 2*Pi]
        vector2 v;
        v.x = std::cos(random);
        v.y = std::sin(random);
        return v;
    }

    // Computes the dot product of the distance and gradient vectors.
    double dotGridGradient(int ix, int iy, double x, double y) const {
        // Get gradient from integer coordinates
        vector2 gradient = randomGradient(ix, iy);

        // Compute the distance vector
        double dx = x - (double)ix;
        double dy = y - (double)iy;

        // Compute the dot-product
        return dx * gradient.x + dy * gradient.y;
    }

    // Compute Perlin noise at coordinates x, y
    double perlin(double x, double y) const {
        // Determine grid cell coordinates
        int x0 = (int)floor(x);
        int x1 = x0 + 1;
        int y0 = (int)floor(y);
        int y1 = y0 + 1;

        // Determine interpolation weights
        // Could also use higher order polynomial/s-curve here
        double sx = x - (double)x0;
        double sy = y - (double)y0;

        // Interpolate between grid point gradients
        double n0, n1, ix0, ix1, value;

        n0 = dotGridGradient(x0, y0, x, y);
        n1 = dotGridGradient(x1, y0, x, y);
        ix0 = interpolate(n0, n1, sx);

        n0 = dotGridGradient(x0, y1, x, y);
        n1 = dotGridGradient(x1, y1, x, y);
        ix1 = interpolate(n0, n1, sx);

        value = interpolate(ix0, ix1, sy);
        return value; // Will return in range -1 to 1. To make it in range 0 to
                      // 1, multiply by 0.5 and add 0.5
    }
};