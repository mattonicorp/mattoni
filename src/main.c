#include <stdlib.h>
#include <SDL2/SDL.h>

#define WINDOW_WIDTH 1300
#define WINDOW_HEIGHT 700

int main() {
    SDL_Event event;
    SDL_Window *window;
    SDL_Renderer *renderer;

    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

    for (int i=0; i<WINDOW_WIDTH; ++i) {
        for (int j=0; j<WINDOW_HEIGHT; ++j) {
            SDL_RenderDrawPoint(renderer, i, j);
        }
    }
    SDL_RenderPresent(renderer);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    for (int i=0; i<WINDOW_WIDTH; ++i) {
        SDL_RenderDrawPoint(renderer, i, 300 + WINDOW_HEIGHT);
    }
    SDL_RenderPresent(renderer);


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
