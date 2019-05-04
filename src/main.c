#include <stdlib.h>
#include <pthread.h>
#include <SDL2/SDL.h>

#include "fractal.h"

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 600

#define HORIZONTAL_REGIONS 4
#define VERTICAL_REGIONS 4

void fractal_worker(ld_complex_t region_top, ld_complex_t region_bot);

// TODO: put that in another file or something
void set_pixel(SDL_Surface *surface, int x, int y, Uint32 value) {
    
    if (x >= surface->w || y >= surface->h) {
        printf("Error: trying to set pixel outside of surface bounds!\n");
        return;
    }

    Uint32 *p = (Uint32 *)surface->pixels + y * surface->w + x;
    *p = value;
}

int main() {

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Error initializing SDL: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    SDL_Window *window;
    window = SDL_CreateWindow("Mattoni: Free Fractals for Everyone lol",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (window == NULL) {
        printf("Error creating window: %s\n", SDL_GetError());
        goto bail_window;
    }

    SDL_Surface *screen_surface; // The main surface on which everything is drawn.
    screen_surface = SDL_GetWindowSurface(window);
    SDL_FillRect(screen_surface, NULL, SDL_MapRGB(screen_surface->format, 0, 0, 0)); // TODO: remove this. this only fill the surface with black but once we get the workers going it wont be needed.

    SDL_LockSurface(screen_surface);

    ld_complex_t top = -2.5 + 1.0I;
    ld_complex_t bottom = 1.0 - 1.0I;
    struct buffer_t *buf = make_buffer(WINDOW_WIDTH, WINDOW_HEIGHT);
    mandelbrot(top, bottom, buf);

    SDL_Color col;
    for (int i=0; i<WINDOW_WIDTH; ++i) {
        for (int j=0; j<WINDOW_HEIGHT; ++j) {
            col = buf->colors[i + j*buf->width];
            set_pixel(screen_surface, i, j, SDL_MapRGB(screen_surface->format, col.r, col.g, col.b));
        }
    }
    SDL_UnlockSurface(screen_surface);

    ld_complex_t viewport_top = CMPLXL(-2.0, 2.0);
    ld_complex_t viewport_bot = CMPLXL(2.0, -2.0);

    // For each region of the screen, create a fractal worker. Each worker will compute the part of
    // the fractal living in the region of the screen it is assigned by putting colors into a buffer.
    long double region_w = (creall(viewport_bot) - creall(viewport_top)) / HORIZONTAL_REGIONS;
    long double region_h = (cimagl(viewport_top) - cimagl(viewport_bot)) / VERTICAL_REGIONS;
    for (int i = 0; i < HORIZONTAL_REGIONS; i++) {
        for (int j = 0; j < VERTICAL_REGIONS; j++) {

            // 'Top' is top-left while 'bottom' ('bot') is bottom-right.
            ld_complex_t region_top = CMPLXL(i * region_w, j * region_h);
            ld_complex_t region_bot = CMPLXL(i * region_w + region_w, j * region_h + region_h);

            fractal_worker(region_top, region_bot);
        }
    }

    SDL_UpdateWindowSurface(window);

    SDL_Event event;
    while (1) {
        if (SDL_PollEvent(&event) && event.type == SDL_QUIT) {
            break;
        }
    }

    SDL_DestroyWindow(window);
    bail_window:
    SDL_Quit;

    return EXIT_SUCCESS;
}

void fractal_worker(ld_complex_t region_top, ld_complex_t region_bot) {
    // 1. Create buffer.
    // 2. Use fractal driver to fill it with beautiful colors.
    // 3. Map the buffer to the SDL buffer.
    // 4. Delete buffer.
    // 5. Die.
//    struct buffer_t *buffer = make_buffer(/* we need to have the size of the region in pixels */);
//    SDL_Surface *region_surface = SDL_CreateRGBSurfaceFrom(
//        (void *)buffer->colors,
//        /* int width, int height */, 8
//    );
//
    return;
}

