#include <stdlib.h>
#include <SDL2/SDL.h>

#include "fractal.h"
#include "pixel_ops.h"
#include "pthread_pool.h"

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 600

#define HORIZONTAL_REGIONS 4
#define VERTICAL_REGIONS 4

/* This is the main screen surface, on which everything is going to be drawn */
SDL_Surface *g_screen_surface;

struct worker_luggage_t {
    ld_complex_t region_top;
    ld_complex_t region_bot;
    SDL_Rect region_pixel_geometry;
    int wew;
};

void *fractal_worker(void *luggage_v);

int main() {

    // Init SDL and stuff.
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Error initializing SDL: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    SDL_Window *window;
    window = SDL_CreateWindow("Mattoni: Free Fractals for Everyone lol",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
    );

    if (window == NULL) {
        printf("Error creating window: %s\n", SDL_GetError());
        goto bail_window;
    }

    g_screen_surface = SDL_GetWindowSurface(window);
    
    // Create a pool thread to contain our workers.
    void *pool = pool_start(fractal_worker, HORIZONTAL_REGIONS * VERTICAL_REGIONS);

    ld_complex_t viewport_top = CMPLXL(-2.5, 1.0);
    ld_complex_t viewport_bot = CMPLXL(1.0, -1.0);



    // For each region of the screen, create a fractal worker. Each worker will compute the part of
    // the fractal living in the region of the screen it is assigned by putting colors into a buffer.
    long double region_w = (creall(viewport_bot) - creall(viewport_top)) / HORIZONTAL_REGIONS;
    long double region_h = (cimagl(viewport_top) - cimagl(viewport_bot)) / VERTICAL_REGIONS;
    for (int i = 0; i < HORIZONTAL_REGIONS; i++) {
        for (int j = 0; j < VERTICAL_REGIONS; j++) {

            size_t region_pixel_width = WINDOW_WIDTH / HORIZONTAL_REGIONS;
            size_t region_pixel_height = WINDOW_HEIGHT / VERTICAL_REGIONS;
            unsigned int px = i * region_pixel_width;
            unsigned int py = j * region_pixel_height;
            SDL_Rect geometry = {px, py, region_pixel_width, region_pixel_height};
            
            struct worker_luggage_t *luggage = malloc(sizeof (struct worker_luggage_t));
            luggage->region_pixel_geometry = geometry;

            // 'Top' is top-left while 'bottom' ('bot') is bottom-right.
            luggage->region_top = viewport_top + CMPLXL(i * region_w, -j * region_h);
            luggage->region_bot = viewport_top + CMPLXL((i+1) * region_w, -(j+1) * region_h);
            static wew = 0;
            wew += 50;
            luggage->wew = wew;

    //        pool_enqueue(pool, (void *)luggage, 1);
        }
    }

    ld_complex_t top = -2.5 + 1.0*I;
    ld_complex_t bottom = 1.0 + -1.0*I;
    struct buffer_t *buf = make_buffer(WINDOW_WIDTH, WINDOW_HEIGHT);
    mandelbrot(top, bottom, buf);
    SDL_Color col;

    for (int i=0; i<WINDOW_WIDTH; ++i) {
        for (int j=0; j<WINDOW_HEIGHT; ++j) {
            col = buf->colors[i + j*buf->width];
            // printf("%d %d %d\n", col.r, col.g, col.b);
            set_pixel(g_screen_surface, i, j, SDL_MapRGB(g_screen_surface->format, col.r, col.g, col.b));
        }
    }

    pool_wait(pool);

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

void *fractal_worker(void *luggage_v) {

    struct worker_luggage_t *luggage = (struct worker_luggage_t *)luggage_v;

    int px = luggage->region_pixel_geometry.x;
    int py = luggage->region_pixel_geometry.y;
    int pw = luggage->region_pixel_geometry.w;
    int ph = luggage->region_pixel_geometry.h;

    SDL_Surface *worker_surface = SDL_CreateRGBSurface(0, pw, ph, 32, 0, 0, 0, 0);

    int wew = luggage->wew;
    SDL_FillRect(worker_surface, NULL, SDL_MapRGB(worker_surface->format, 0, 255, wew));

    SDL_BlitSurface(worker_surface, NULL, g_screen_surface, &luggage->region_pixel_geometry);

    return NULL;
}

