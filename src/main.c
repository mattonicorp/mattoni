#include <stdlib.h>
#include <pthread.h>
#include <SDL2/SDL.h>

#include "fractal.h"

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 600

#define HORIZONTAL_SCREEN_REGIONS 4
#define VERTICAL_SCREEN_REGIONS 4

void fractal_worker(ld_complex_t region_top, ld_complex_t region_bot);

int main() {

    SDL_Event event;
    SDL_Window *window;
    SDL_Renderer *renderer;

    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);

    ld_complex_t viewport_top = CMPLXL(-2.0, 2.0);
    ld_complex_t viewport_bot = CMPLXL(2.0, -2.0);

    // For each region of the screen, create a fractal worker. Each worker will compute the part of
    // the fractal living in the region of the screen it is assigned by putting colors into a buffer.
    long double region_w = (creall(viewport_bot) - creall(viewport_top)) / HORIZONTAL_SCREEN_REGIONS;
    long double region_h = (cimagl(viewport_top) - cimagl(viewport_bot)) / VERTICAL_SCREEN_REGIONS;
    for (int i = 0; i < HORIZONTAL_SCREEN_REGIONS; i++) {
        for (int j = 0; j < VERTICAL_SCREEN_REGIONS; j++) {

            // 'Top' is top-left while 'bottom' ('bot') is bottom-right.
            ld_complex_t region_top = CMPLXL(i * region_w, j * region_h);
            ld_complex_t region_bot = CMPLXL(i * region_w + region_w, j * region_h + region_h);

            fractal_worker(region_top, region_bot);
        }
    }

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    for (int i=0; i<1000; ++i) {
        for (int j=0; j<WINDOW_HEIGHT; ++j) {
            SDL_RenderDrawPoint(renderer, i, j);
        }
    }
    SDL_RenderPresent(renderer);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    for (int i=0; i<WINDOW_WIDTH; ++i) {
    }


    while (1) {
        if (SDL_PollEvent(&event) && event.type == SDL_QUIT) {
            break;
        }
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit;

    return EXIT_SUCCESS;
}

void fractal_worker(ld_complex_t region_top, ld_complex_t region_bot) {
    // 1. Create buffer.
    // 2. Use fractal driver to fill it with beautiful colors.
    // 3. Map the buffer to the SDL buffer.
    // 4. Delete buffer.
    // 5. Die.
    return;
}

