#include <stdlib.h>
#include <pthread.h>
#include <SDL2/SDL.h>

#include "fractal.h"
#include "pixel_ops.h"
#include "pthread_pool.h"

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 600

#define HORIZONTAL_REGIONS 16
#define VERTICAL_REGIONS 16

/* Contains data to send to fractal workers. */
struct worker_luggage_t {
    ld_complex_t region_top;
    ld_complex_t region_bot;
    SDL_Rect region_pixel_geometry;
};

SDL_Surface *g_screen_surface;
SDL_Surface *g_worker_surface;
pthread_mutex_t g_worker_surface_lock = PTHREAD_MUTEX_INITIALIZER;

/* This is a thread pool that will contain our workers. */
void *g_pool;

void draw_fractal(ld_complex_t viewport_top, ld_complex_t viewport_bot);
void *fractal_worker(void *luggage_v);
void change_viewport(int down_x, int down_y, int up_x, int up_y,
                     ld_complex_t *viewport_top, ld_complex_t *viewport_bottom);
void change_centre(int centre_x, int centre_y, ld_complex_t *viewport_top, ld_complex_t *viewport_bottom);

int main() {

    // Init SDL and stuff.
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Error initializing SDL: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    SDL_Window *window;
    window = SDL_CreateWindow("Mattoni: Free Fractals for the People",
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
    g_worker_surface = SDL_CreateRGBSurface(0,
        WINDOW_WIDTH / HORIZONTAL_REGIONS, WINDOW_HEIGHT / VERTICAL_REGIONS,
        32, 0, 0, 0, 0);

    // This is the initial viewport. The viewport is like a window into the complex plane. It has
    // a fixed shape but it is independant of the actual window (and window's surface) size. One
    // major difference is that here the coordinate system is like in the complex plane (increasing y
    // values go 'up') while the SDL coordinate system is different (increasing y go 'down').
    ld_complex_t viewport_top = CMPLXL(-2.5, 1.0);
    ld_complex_t viewport_bot = CMPLXL(1.0, -1.0);

    SDL_Event event;
    int down_x, down_y;
    int up_x, up_y;
    int curr_x, curr_y;
    int mouse_down = 0;
    static int dirty = 1;
    while (1) {
        // SDL_GetMouseState(&curr_x, &curr_y);
        // printf("Curr pos: %d %d\n", curr_x, curr_y);

        if (dirty) {
            /* Ooh, she be dirty */
            dirty = 0;
            printf("Redrawing fractal\n");
            draw_fractal(viewport_top, viewport_bot);
        }

        SDL_UpdateWindowSurface(window);

        if (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    goto exit_routine;
                case SDL_MOUSEBUTTONDOWN:
                    SDL_GetMouseState(&down_x, &down_y);
                    break;
                case SDL_MOUSEBUTTONUP:
                    SDL_GetMouseState(&up_x, &up_y);
                    printf("Mouse down: %d %d\n", down_x, down_y);
                    printf("Mouse up: %d %d\n", up_x, up_y);
                    change_viewport(down_x, down_y, up_x, up_y, &viewport_top, &viewport_bot);
                    printf("Viewport top: %LG %LG\n", creall(viewport_top), cimagl(viewport_top));
                    printf("Viewport bottom: %LG %LG\n", creall(viewport_bot), cimagl(viewport_bot));
                    dirty = 1;
                    break;
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        /* move by sending specially chosen boundaries to change_viewport */
                        case SDLK_LEFT:
                            change_centre(0, WINDOW_HEIGHT/2, &viewport_top, &viewport_bot);
                            goto set_dirty;
                        case SDLK_RIGHT:
                            change_centre(WINDOW_WIDTH, WINDOW_HEIGHT/2, &viewport_top, &viewport_bot);
                            goto set_dirty;
                        case SDLK_UP:
                            change_centre(WINDOW_WIDTH/2, 0, &viewport_top, &viewport_bot);
                            goto set_dirty;
                        case SDLK_DOWN:
                            change_centre(WINDOW_WIDTH/2, WINDOW_HEIGHT, &viewport_top, &viewport_bot);
                            goto set_dirty;
                        default:
                            break;
                        set_dirty:
                            dirty = 1;

                    }
                    break;

            }
        }
    }

    exit_routine:
    SDL_FreeSurface(g_worker_surface);
    SDL_DestroyWindow(window);
    bail_window:
    SDL_Quit;

    return EXIT_SUCCESS;
}

void draw_fractal(ld_complex_t viewport_top, ld_complex_t viewport_bot) {

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

    // This is the long computation part.
    struct buffer_t *buf = make_buffer(pw, ph);
    julia(region_top, region_bot, 2, buf);

    // Lock the global worker surface before copying the buffer on it because it is shared by all the threads.
    pthread_mutex_lock(&g_worker_surface_lock);
    for (int x = 0; x < pw; x++) {
        for (int y = 0; y < ph; y++) {
            SDL_Color col = buf->colors[x + y * pw];
            set_pixel(g_worker_surface, x, y, SDL_MapRGB(g_worker_surface->format, col.r, col.g, col.b));
        }
    }
    SDL_BlitSurface(g_worker_surface, NULL, g_screen_surface, &luggage->region_pixel_geometry);
    pthread_mutex_unlock(&g_worker_surface_lock);

    free(buf);
    return NULL;
}

void change_viewport(int down_x, int down_y, int up_x, int up_y,
                     ld_complex_t *viewport_top, ld_complex_t *viewport_bottom) {
    int top_x = (down_x < up_x) ? down_x : up_x;
    int top_y = (down_y < up_y) ? down_y : up_y;
    int bottom_x = (down_x < up_x) ? up_x : down_x;
    int bottom_y = (down_y < up_y) ? up_y : down_y;

    int centre_x = (top_x + bottom_x) / 2;
    int centre_y = (top_y + bottom_y) / 2;

    float factor = (float) (bottom_x - top_x) / WINDOW_HEIGHT;
    factor = (factor < 0.06) ? 0.06 : factor;

    long double real_width = (creall(*viewport_bottom) - creall(*viewport_top));
    long double imag_height = (cimagl(*viewport_bottom) - cimagl(*viewport_top));

    printf("New width: %LG. New height: %LG.\n", real_width, imag_height);

    long double real_offset = ((long double) centre_x / WINDOW_WIDTH)*real_width;
    long double imag_offset = ((long double) centre_y / WINDOW_HEIGHT)*imag_height;

    long double new_centre_real = real_offset + creall(*viewport_top);
    long double new_centre_imag = imag_offset + cimagl(*viewport_top);

    printf("Centre: %LG %LG\n", new_centre_real, new_centre_imag);

    long double new_top_real = new_centre_real - factor*real_width/2;
    long double new_top_imag = new_centre_imag - factor*imag_height/2;
    long double new_bottom_real = new_centre_real + factor*real_width/2;
    long double new_bottom_imag = new_centre_imag + factor*imag_height/2;

    *viewport_top = CMPLXL(new_top_real, new_top_imag);
    *viewport_bottom = CMPLXL(new_bottom_real, new_bottom_imag);
}

void change_centre(int centre_x, int centre_y, ld_complex_t *viewport_top, ld_complex_t *viewport_bottom) {
    /* copy-paste from above ... I'm not proud */
    long double real_width = (creall(*viewport_bottom) - creall(*viewport_top));
    long double imag_height = (cimagl(*viewport_bottom) - cimagl(*viewport_top));

    long double real_offset = ((long double) centre_x / WINDOW_WIDTH)*real_width;
    long double imag_offset = ((long double) centre_y / WINDOW_HEIGHT)*imag_height;

    long double new_centre_real = real_offset + creall(*viewport_top);
    long double new_centre_imag = imag_offset + cimagl(*viewport_top);

    printf("Centre: %LG %LG\n", new_centre_real, new_centre_imag);

    long double new_top_real = new_centre_real - real_width/2;
    long double new_top_imag = new_centre_imag - imag_height/2;
    long double new_bottom_real = new_centre_real + real_width/2;
    long double new_bottom_imag = new_centre_imag + imag_height/2;

    *viewport_top = CMPLXL(new_top_real, new_top_imag);
    *viewport_bottom = CMPLXL(new_bottom_real, new_bottom_imag);
}

