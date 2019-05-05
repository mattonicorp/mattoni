/* Interface for fractal generation driver */

#include <complex.h>
#include <SDL2/SDL.h>

#include "buffer.h"
#include "mattoni_types.h"

void fractal(ld_complex_t top, ld_complex_t bottom, int which_fractal, unsigned int seed, struct buffer_t *buf);
