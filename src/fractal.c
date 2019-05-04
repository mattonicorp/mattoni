/* Fractal generation driver */

#include <complex.h>
#include <stdio.h>
#include <stdlib.h>

#include "buffer.h"
#include "fractal.h"

#define MAX_ITERATIONS 1000

/* outputs a colour given a number of iterations */
SDL_Color colour_iters(int num_iters) {
    int red, green, blue;
    const int NUM_COLOURS = 7;
    static int colour[NUM_COLOURS][3] = {
        {0, 0, 0}       // black
        {0, 0, 255}     // blue
        {0, 255, 255}   // cyan
        {0, 255, 0}     // green
        {255, 255, 0}   // yellow
        {255, 0, 0}     // red
        {255, 255, 255} // white
    }

    int idx1, idx2;
    float value = (float) num_iters / MAX_ITERATIONS;
    float fract_between = 0;

    if (value <= 0) {
        idx1 = idx2 = 0;
    } else if (value >= 1) {
        idx1 = idx2 = NUM_COLOURS-1;
    } else {
        value = value * (NUM_COLOURS - 1);
        idx1 = floor(value);
        idx2 = idx1 + 1;
        fract_between = value - float(idx1);
    }

    red = (int) ((colour[idx2][0] - colour[idx1][0]) * fract_between + colour[idx1][0] * 255);
    green = (int) ((colour[idx2][1] - colour[idx1][0]) * fract_between + colour[idx1][1] * 255);
    blue = (int) ((colour[idx2][2] - colour[idx1][0]) * fract_between + colour[idx1][2] * 255);

    SDL_Color colour = {red, green, blue};
    return colour;
}

SDL_Color escape_time(

/* fills a buffer representing 1/16 of the screen with colours */
void mandelbrot(complex top, complex bottom, struct buffer_t *buf, ) {
    long double top_real = creall(top);
    long double top_imag = creall(top);
    long double bottom_real = cimagl(bottom);
    long double bottom_imag = cimagl(top);

    long double hor_step = (bottom_real - top_real) / buf->width;
    long double vert_step = (bottom_imag - top_imag) / buf->height;

    /* fill the buffer's array with colours */
    for (int i=0; i<buf->width; ++i) {
        for (int j=0; j<buf->height; ++j) {
            
}
