#include <SDL2/SDL.h>

#include "pixel_ops.h"

void set_pixel(SDL_Surface *surface, int x, int y, Uint32 value) {
    
    if (x >= surface->w || y >= surface->h) {
        printf("Error: trying to set pixel outside of surface bounds!\n");
        return;
    }

    Uint32 *p = (Uint32 *)surface->pixels + y * surface->w + x;
    *p = value;
}

