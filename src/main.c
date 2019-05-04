#include <stdlib.h>
#include <SDL2/SDL.h>

#include "fractal.h"
#include "pixel_ops.h"
#include "pthread_pool.h"

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 600

#define HORIZONTAL_REGIONS 8
#define VERTICAL_REGIONS 8

/* Contains data to send to fractal workers. */
struct worker_luggage_t {
    ld_complex_t region_top;
    ld_complex_t region_bot;
    SDL_Rect region_pixel_geometry;
};

/* This is the main screen surface, on which everything is going to be drawn */
SDL_Surface *g_screen_surface;

/* This is a thread pool that will contain our workers. */
void *g_pool;

void draw(ld_complex_t viewport_top, ld_complex_t viewport_bot);
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

    // Set globals.
    g_screen_surface = SDL_GetWindowSurface(window);
    g_pool = pool_start(fractal_worker, HORIZONTAL_REGIONS * VERTICAL_REGIONS);

    // This is the initial viewport. The viewport is like a window into the complex plane. It has
    // a fixed shape but it is independant of the actual window (and window's surface) size. One
    // major difference is that here the coordinate system is like in the complex plane (increasing y
    // values go 'up') while the SDL coordinate system is different (increasing y go 'down').
    ld_complex_t viewport_top = CMPLXL(-2.5, 1.0);
    ld_complex_t viewport_bot = CMPLXL(1.0, -1.0);

    SDL_Event event;
    int dirty = 1;
    while (1) {

        if (dirty) {
            dirty = 0;
            printf("Redrawing fractal\n");
            draw(viewport_top, viewport_bot);
        }
        SDL_UpdateWindowSurface(window);

        if (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    goto exit_routine;
            }
        }
    }

    exit_routine:
    SDL_DestroyWindow(window);
    bail_window:
    SDL_Quit;

    return EXIT_SUCCESS;
}

void draw(ld_complex_t viewport_top, ld_complex_t viewport_bot) {

    // Each region of the screen has a size as measured in the complex plane and a size in pixels.
    // Both are needed but they are unrelated to each other, despite the very similar names.
    long double region_w = (creall(viewport_bot) - creall(viewport_top)) / HORIZONTAL_REGIONS;
    long double region_h = (cimagl(viewport_top) - cimagl(viewport_bot)) / VERTICAL_REGIONS;
    size_t region_w_p = WINDOW_WIDTH / HORIZONTAL_REGIONS;
    size_t region_h_p = WINDOW_HEIGHT / VERTICAL_REGIONS;

    // For each region of the screen, create a fractal worker. Each worker will compute the part of
    // the fractal living in the region of the screen it is assigned by putting colors into a buffer.
    for (int i = 0; i < HORIZONTAL_REGIONS; i++) {
        for (int j = 0; j < VERTICAL_REGIONS; j++) {

            // The following lines are to compute the position of each region in PIXELS. This is
            // for RENDERING and is not related to the actual computation of fractals.
            unsigned int px = i * region_w_p;
            unsigned int py = j * region_h_p;
            SDL_Rect geometry = {px, py, region_w_p, region_h_p};

            // Put all the info needed by each worker into a 'luggage'. No mem leak as pthread_pool
            // will free everything once the task of a worker is done.
            struct worker_luggage_t *luggage = malloc(sizeof (struct worker_luggage_t));
            luggage->region_pixel_geometry = geometry;
            luggage->region_top = viewport_top + CMPLXL(i * region_w, -j * region_h);
            luggage->region_bot = viewport_top + CMPLXL((i+1) * region_w, -(j+1) * region_h);

            // Send the task to the pool, let some worker take care of it (for free; I love slavery).
            pool_enqueue(g_pool, (void *)luggage, 1);
        }
    }

    // Wait until all workers are done rendering their part of the fractal.
    //pool_wait(g_pool);
}

void *fractal_worker(void *luggage_v) {

    // Need to cast the luggage into its real type. All this deal with void pointers and struct luggage
    // is because a fractal worker can only receive one parameter, so we gotta bundle all the params
    // we want to send into one big struct and also put it on the heap (no shared stack for threads!)
    struct worker_luggage_t *luggage = (struct worker_luggage_t *)luggage_v;

    // Unpack the luggage.
    int px = luggage->region_pixel_geometry.x;
    int py = luggage->region_pixel_geometry.y;
    int pw = luggage->region_pixel_geometry.w;
    int ph = luggage->region_pixel_geometry.h;

    ld_complex_t region_top = luggage->region_top;
    ld_complex_t region_bot = luggage->region_bot;

    SDL_Surface *worker_surface = SDL_CreateRGBSurface(0, pw, ph, 32, 0, 0, 0, 0);

    struct buffer_t *buf = make_buffer(pw, ph);
    mandelbrot(region_top, region_bot, buf);
    for (int x = 0; x < pw; x++) {
        for (int y = 0; y < ph; y++) {
            SDL_Color col = buf->colors[x + y * pw];
            set_pixel(worker_surface, x, y, SDL_MapRGB(worker_surface->format, col.r, col.g, col.b));
        }
    }

    SDL_BlitSurface(worker_surface, NULL, g_screen_surface, &luggage->region_pixel_geometry);

    // The memory leak-free corner of this code :^)
    free(buf);
    SDL_FreeSurface(worker_surface);

    return NULL;
}

