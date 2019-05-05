/* Fractal generation driver */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <complex.h>

#include <mpfr.h>
#include <mpc.h>

#include "buffer.h"
#include "fractal.h"

#define MAX_ITERATIONS 400
#define NUM_COLOURS 9

// These different functions (have the same signature) will compute different fractals.
void mandelbrot(mpc_t top, mpc_t bottom, unsigned int seed, struct buffer_t *buf);
void julia(mpc_t top, mpc_t bottom, unsigned int seed, struct buffer_t *buf);
void ship(mpc_t top, mpc_t bottom, unsigned int seed, struct buffer_t *buf);

// How to die:
typedef void (*fractal_fn)(mpc_t, mpc_t, unsigned int, struct buffer_t *);
fractal_fn fractal_types[3] = {
    &mandelbrot,
    &julia,
    &ship
};

void fractal(mpc_t top, mpc_t bottom, unsigned int seed, struct buffer_t *buf) {

    // Depending on the seed, we choose a different type of fractal.
    fractal_fn f = fractal_types[seed % 3];
    f(top, bottom, seed, buf);
}

/* outputs a colour given a number of iterations */
SDL_Color colour_iters(unsigned int num_iters) {
    int red, green, blue;
    static float colour[NUM_COLOURS][3] = {
        {0, 0, 0}, // black
        {0.501, 0.164, 0.074}, // brown
        {1, 0.5, 0}, // orange
        {1, 1, 0}, // yellow
        {0.101, 0.501, 0}, // green
        {0, 1, 1}, // cyan
        {0, 0, 1}, // blue
        {0.25, 0, 1}, // purple
        {0, 0, 0}, // black
    };

    int idx1, idx2;
    float value = (float) num_iters / MAX_ITERATIONS;
    float fract_between = 0;

    if (value <= 0) {
        idx1 = idx2 = 0;
    } else if (value >= 1) {
        idx1 = idx2 = NUM_COLOURS-1;
    } else {
        /* this is the most likely case */
        value = value * (NUM_COLOURS - 1);
        idx1 = floor(value);
        idx2 = idx1 + 1;
        fract_between = value - (float) idx1;
    }

    red = (int) (((colour[idx2][0] - colour[idx1][0]) * fract_between + colour[idx1][0]) * 255);
    green = (int) (((colour[idx2][1] - colour[idx1][1]) * fract_between + colour[idx1][1]) * 255);
    blue = (int) (((colour[idx2][2] - colour[idx1][2]) * fract_between + colour[idx1][2]) * 255);

    SDL_Color ret_val = {red, green, blue};
    return ret_val;
}

/* linear interpolation of two colours */
SDL_Color lerp(SDL_Color c1, SDL_Color c2, float t) {
    SDL_Color new_colour;
    new_colour.r = c1.r + (c2.r - c1.r) * t;
    new_colour.g = c1.g + (c2.g - c1.g) * t;
    new_colour.b = c1.b + (c2.b - c1.b) * t;
    new_colour.a = c1.a + (c2.a - c1.a) * t;

    return new_colour;
}

SDL_Color get_color(mpc_t z, unsigned int iteration) {

//    long double x = creall(z);
//    long double y = cimagl(z);
//
//    long double nu;
//    float flt_iter;
//    if (iteration < MAX_ITERATIONS) {
//        long double log_zn = log(x*x + y*y) / 2;
//        nu = log (log_zn / log(2)) / log(2);
//        flt_iter = iteration + 1.0 - nu;
//    } else {
//        flt_iter = MAX_ITERATIONS - 1;
//    }
//
//    SDL_Color col1 = colour_iters((unsigned int) flt_iter);
//    SDL_Color col2 = colour_iters((unsigned int) flt_iter + 1);
//    float fract = fmod(flt_iter, 1);
//
//    return lerp(col1, col2, fract);
    return colour_iters(iteration);
}

void ship(mpc_t top, mpc_t bottom, unsigned int seed, struct buffer_t *buf) {

//    long double step_w = (creall(bottom) - creall(top)) / buf->width;
//    long double step_h = (cimagl(bottom) - cimagl(top)) / buf->height;
//    for (unsigned int i = 0; i < buf->width; i++) {
//        for (unsigned int j = 0; j < buf->height; j++) {
//            
//            unsigned int iteration = 0;
//            
//            ld_complex_t z = CMPLXL(0.0, 0.0);
//            ld_complex_t c = top + CMPLXL(i * step_w, j * step_h);
//            while (cabs(z) <= 2.0 && iteration < MAX_ITERATIONS) {
//                z = CMPLXL(creall(z)*creall(z) - cimagl(z)*cimagl(z), 2.0*fabsl(z)*creall(z));
//                z += c;
//                iteration++;
//            }
//
//            set_color(buf, i, j, get_color(z, iteration));
//        }
//    }
}

void julia(mpc_t top, mpc_t bottom, unsigned int seed, struct buffer_t *buf) {

    long double complex vals[4] = {
        CMPLXL(-0.8, 0.156),
        CMPLXL(-0.4, 0.6),
        CMPLXL(0.285, 0.01),
        CMPLXL(-0.7269, 0.1889)
    };

    mpc_t c, z;
    mpc_init2(c, 256);
    mpc_init2(z, 256);
    mpc_set_ldc(c, vals[0], MPC_RNDNN);

    mpfr_t step_w, step_h, t1, t2;
    mpfr_inits2(256, step_w, step_h, t1, t2, (mpfr_ptr)0);
    mpfr_sub(step_w, mpc_realref(bottom), mpc_realref(top), MPFR_RNDN);
    mpfr_sub(step_h, mpc_imagref(bottom), mpc_imagref(top), MPFR_RNDN);
    mpfr_div_ui(step_w, step_w, buf->width, MPFR_RNDN);
    mpfr_div_ui(step_h, step_h, buf->height, MPFR_RNDN);

    for (unsigned int i = 0; i < buf->width; i++) {
        for (unsigned int j = 0; j < buf->height; j++) {

            unsigned int iteration = 0;

            // Compute the current complex point, where we start iterating.
            mpfr_mul_ui(t1, step_w, i, MPFR_RNDN);
            mpfr_mul_ui(t2, step_h, j, MPFR_RNDN);
            mpc_set_fr_fr(z, t1, t2, MPFR_RNDN);
            mpc_add(z, z, top, MPC_RNDNN);

            while (iteration < MAX_ITERATIONS) {

                mpc_norm(t1, z, MPFR_RNDN);
                if (mpfr_cmp_d(t1, 4.0) > 0) {
                    break;
                }

                mpc_sqr(z, z, MPC_RNDNN);
                mpc_add(z, z, c, MPC_RNDNN);

                iteration++;
            }

            set_color(buf, i, j, get_color(z, iteration));
        }
    }

    mpfr_clears(step_w, step_h, t1, t2, (mpfr_ptr)0);
    mpc_clear(c);
    mpc_clear(z);
}

void mandelbrot(mpc_t top, mpc_t bottom, unsigned int seed, struct buffer_t *buf) {

//    long double step_w = (creall(bottom) - creall(top)) / buf->width;
//    long double step_h = (cimagl(bottom) - cimagl(top)) / buf->height;
//    for (unsigned int i=0; i<buf->width; ++i) {
//        for (unsigned int j=0; j<buf->height; ++j) {
//
//            unsigned int iteration = 0;
//
//            ld_complex_t z = CMPLXL(0.0, 0.0);
//            ld_complex_t c = top + CMPLXL(i * step_w, j * step_h);
//            while (cabsl(z) <= 2.0 && iteration < MAX_ITERATIONS) {
//                z = z*z + c;
//                iteration++;
//            }
//
//            set_color(buf, i, j, get_color(z, iteration));
//        }
//    }
}

