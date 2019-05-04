/* Interface for fractal generation driver */

#include <complex.h>
#include <SDL2/SDL.h>

#include "buffer.h"

typedef long double complex ld_complex_t;

/* outputs a colour given a number of iterations */
SDL_Color colour_iters(int num_iters);

/* fills a buffer representing 1/16 of the screen with colours */
void mandelbrot(ld_complex_t top, ld_complex_t bottom, struct buffer_t *buf);
