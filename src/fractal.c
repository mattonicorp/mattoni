/* Fractal generation driver */

#include <complex.h>
#include <stdio.h>
#include <stdlib.h>

#include "buffer.h"
#include "fractal.h"

#define MAX_ITERATIONS 100
#define NUM_COLOURS 13

/* outputs a colour given a number of iterations */
SDL_Color colour_iters(unsigned int num_iters) {
    int red, green, blue;
    static float colour[NUM_COLOURS][3] = {
        {1, 0.5, 0}, // orange
        {1, 1, 0}, // yellow
        {0, 1, 0}, // green
        {0, 1, 1}, // cyan
        {0, 0, 1}, // blue
        {0.5, 0, 1}, // purple
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

/* fills a buffer representing 1/16 of the screen with colours */
void mandelbrot(ld_complex_t top, ld_complex_t bottom, struct buffer_t *buf) {
    long double top_real = creall(top);
    long double top_imag = cimagl(top);
    long double bottom_real = creall(bottom);
    long double bottom_imag = cimagl(bottom);

    long double hor_step = (bottom_real - top_real) / buf->width;
    long double vert_step = (bottom_imag - top_imag) / buf->height;

    /* fill the buffer's array with colours */
    long double real_part = top_real;
    long double imag_part = 0.0;
    for (unsigned int i=0; i<buf->width; ++i) {
        long double imag_part = top_imag;
        for (unsigned int j=0; j<buf->height; ++j) {
            long double x = 0.0;
            long double y = 0.0;
            unsigned int iteration = 0;

            while (x*x + y*y <= 2*2 && iteration < MAX_ITERATIONS) {
                long double xtemp = x*x - y*y + real_part;
                y = 2.0*x*y + imag_part;
                x = xtemp;
                ++iteration;
            }

            // printf("%LG %LG, %u\n", real_part, imag_part, iteration);

            SDL_Color col = colour_iters(iteration);

            set_color(buf, i, j, col);

            imag_part += vert_step;
        }
        real_part += hor_step;
    }
}
