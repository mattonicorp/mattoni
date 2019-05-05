#include <stdlib.h>
#include <stdio.h>
#include <complex.h>
#include <pthread.h>

#include <mpfr.h>
#include <mpc.h>

#include <SDL2/SDL.h>

#include "fractal.h"
#include "pixel_ops.h"
#include "pthread_pool.h"

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 600

#define HORIZONTAL_REGIONS 2
#define VERTICAL_REGIONS 2

/* Contains data to send to fractal workers. */
struct worker_luggage_t {
    char *region_top_str;
    char *region_bot_str;
    SDL_Rect region_pixel_geometry;
};

SDL_Surface *g_screen_surface;
SDL_Surface *g_worker_surface;
pthread_mutex_t g_worker_surface_lock = PTHREAD_MUTEX_INITIALIZER;

/* This is a thread pool that will contain our workers. */
void *g_pool;

void draw_fractal(mpc_t viewport_top, mpc_t viewport_bot);
void *fractal_worker(void *luggage_v);
void change_viewport(int down_x, int down_y, int up_x, int up_y,
                     ld_complex_t *viewport_top, ld_complex_t *viewport_bottom);
void change_centre(int centre_x, int centre_y, ld_complex_t *viewport_top, ld_complex_t *viewport_bottom);
void zoom(float factor, ld_complex_t *viewport_top, ld_complex_t *viewport_bottom);

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
    mpc_t viewport_top, viewport_bot;
    mpc_init2(viewport_top, 256);
    mpc_init2(viewport_bot, 256);
    mpc_set_dc(viewport_top, CMPLX(-2.5, 1.0), MPC_RNDNN);
    mpc_set_dc(viewport_bot, CMPLX(1.0, -1.0), MPC_RNDNN);

    SDL_Event event;
    int down_x, down_y;
    int up_x, up_y;
    int curr_x, curr_y;
    int mouse_down = 0;
    static int dirty = 1;
    while (1) {

        if (dirty) {
            pool_wait(g_pool);
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
            }
        }
//                case SDL_MOUSEBUTTONDOWN:
//                    SDL_GetMouseState(&down_x, &down_y);
//                    break;
//                case SDL_MOUSEBUTTONUP:
//                    SDL_GetMouseState(&up_x, &up_y);
//                    printf("Mouse down: %d %d\n", down_x, down_y);
//                    printf("Mouse up: %d %d\n", up_x, up_y);
//                    change_viewport(down_x, down_y, up_x, up_y, &viewport_top, &viewport_bot);
//                    printf("Viewport top: %LG %LG\n", creall(viewport_top), cimagl(viewport_top));
//                    printf("Viewport bottom: %LG %LG\n", creall(viewport_bot), cimagl(viewport_bot));
//                    dirty = 1;
//                    break;
//                case SDL_KEYDOWN:
//                    switch (event.key.keysym.sym) {
//                        /* move by sending specially chosen boundaries to change_viewport */
//                        case SDLK_h:
//                        case SDLK_LEFT:
//                            change_centre(0, WINDOW_HEIGHT/2, &viewport_top, &viewport_bot);
//                            goto do_the_dirty;
//                        case SDLK_l:
//                        case SDLK_RIGHT:
//                            change_centre(WINDOW_WIDTH, WINDOW_HEIGHT/2, &viewport_top, &viewport_bot);
//                            goto do_the_dirty;
//                        case SDLK_k:
//                        case SDLK_UP:
//                            change_centre(WINDOW_WIDTH/2, 0, &viewport_top, &viewport_bot);
//                            goto do_the_dirty;
//                        case SDLK_j:
//                        case SDLK_DOWN:
//                            change_centre(WINDOW_WIDTH/2, WINDOW_HEIGHT, &viewport_top, &viewport_bot);
//                            goto do_the_dirty;
//                        case SDLK_SPACE: // zoom out
//                        case SDLK_n:
//                            zoom(2.0, &viewport_top, &viewport_bot);
//                            goto do_the_dirty;
//                        case SDLK_RETURN: // zoom in
//                        case SDLK_u:
//                            zoom(0.5, &viewport_top, &viewport_bot);
//                            goto do_the_dirty;
//                        default:
//                            break;
//                        do_the_dirty:
//                            dirty = 1;
//
//                    }
//                    break;
//
//            }
//        }
    }

    exit_routine:
    mpc_clear(viewport_top);
    mpc_clear(viewport_bot);
    SDL_FreeSurface(g_worker_surface);
    SDL_DestroyWindow(window);
    bail_window:
    SDL_Quit;

    return EXIT_SUCCESS;
}

void draw_fractal(mpc_t viewport_top, mpc_t viewport_bot) {

    // Compute the size (width and height) of each region of the screen in terms of complex numbers.
    mpfr_t region_w, region_h, t1, t2;
    mpfr_inits2(256, region_w, region_h, t1, t2, (mpfr_ptr)0);
    mpfr_sub(region_w, mpc_realref(viewport_bot), mpc_realref(viewport_top), MPFR_RNDN);
    mpfr_sub(region_h, mpc_imagref(viewport_bot), mpc_imagref(viewport_top), MPFR_RNDN);
    mpfr_div_ui(region_w, region_w, (unsigned long int)HORIZONTAL_REGIONS, MPFR_RNDN);
    mpfr_div_ui(region_h, region_h, (unsigned long int)VERTICAL_REGIONS, MPFR_RNDN);

    mpc_t top, bot;
    mpc_init2(top, 256);
    mpc_init2(bot, 256);

    // Compute the size of each region again but this time in terms of pixels!
    size_t region_w_p = WINDOW_WIDTH / HORIZONTAL_REGIONS;
    size_t region_h_p = WINDOW_HEIGHT / VERTICAL_REGIONS;

    // For each region of the screen, create a fractal worker. Each worker will compute the part of
    // the fractal living in the region of the screen it is assigned by putting colors into a buffer.
    for (unsigned long int i = 0; i < HORIZONTAL_REGIONS; i++) {
        for (unsigned long int j = 0; j < VERTICAL_REGIONS; j++) {

            // The following lines are to compute the position of each region in PIXELS. This is
            // for RENDERING and is not related to the actual computation of fractals.
            unsigned int px = i * region_w_p;
            unsigned int py = j * region_h_p;
            SDL_Rect geometry = {px, py, region_w_p, region_h_p};

            struct worker_luggage_t *luggage = malloc(sizeof (struct worker_luggage_t));
            luggage->region_pixel_geometry = geometry;
            //luggage->region_top = viewport_top + CMPLXL(i * region_w, -j * region_h);
            //luggage->region_bot = viewport_top + CMPLXL((i+1) * region_w, -(j+1) * region_h);

            // Compute the bottom-right and top-left points of the current region (as complex numbers).
            mpfr_mul_ui(t1, region_w, i, MPFR_RNDN);
            mpfr_mul_ui(t2, region_h, j, MPFR_RNDN);
            mpc_set_fr_fr(top, t1, t2, MPFR_RNDN);
            mpc_add(top, top, viewport_top, MPFR_RNDN);

            mpfr_mul_ui(t1, region_w, i+1, MPFR_RNDN);
            mpfr_mul_ui(t2, region_h, j+1, MPFR_RNDN);
            mpc_set_fr_fr(bot, t1, t2, MPFR_RNDN);
            mpc_add(bot, bot, viewport_top, MPFR_RNDN);

            luggage->region_top_str = mpc_get_str(36, 0, top, MPFR_RNDN);
            luggage->region_bot_str = mpc_get_str(36, 0, bot, MPFR_RNDN);

            // Send the task to the pool, let some worker take care of it (for free; I love slavery).
            // Moreover the worker will automatically free the luggage when it's done.
            pool_enqueue(g_pool, (void *)luggage, 1);
        }
    }

    mpc_clear(bot);
    mpc_clear(top);
    mpfr_clears(region_w, region_h, t1, t2, (mpfr_ptr)0);
}

void *fractal_worker(void *luggage_v) {

    mpc_t region_top, region_bot;
    mpc_init2(region_top, 256);
    mpc_init2(region_bot, 256);

    // Need to cast the luggage into its real type. All this deal with void pointers and struct luggage
    // is because a fractal worker can only receive one parameter, so we gotta bundle all the params
    // we want to send into one big struct and also put it on the heap (no shared stack for threads!)
    struct worker_luggage_t *luggage = (struct worker_luggage_t *)luggage_v;

    mpc_set_str(region_top, luggage->region_top_str, 36, MPC_RNDNN);
    mpc_set_str(region_bot, luggage->region_bot_str, 36, MPC_RNDNN);

    // That memory ain't free, sugar!
    mpc_free_str(luggage->region_top_str);
    mpc_free_str(luggage->region_bot_str);

    int px = luggage->region_pixel_geometry.x;
    int py = luggage->region_pixel_geometry.y;
    int pw = luggage->region_pixel_geometry.w;
    int ph = luggage->region_pixel_geometry.h;

    struct buffer_t *buf = make_buffer(pw, ph);
    fractal(region_top, region_bot, 1, buf);

    // Lock the worker surface before copying the buffer on it because it is shared by all the threads.
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
    mpc_clear(region_top);
    mpc_clear(region_bot);
    
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

void zoom(float factor, ld_complex_t *viewport_top, ld_complex_t *viewport_bottom) {
    long double real_width = (creall(*viewport_bottom) - creall(*viewport_top));
    long double imag_height = (cimagl(*viewport_bottom) - cimagl(*viewport_top));

    printf("New width: %LG. New height: %LG.\n", real_width, imag_height);

    long double centre_real = creall(*viewport_top) + real_width/2;
    long double centre_imag = cimagl(*viewport_top) + imag_height/2;

    printf("Centre: %LG %LG\n", centre_real, centre_imag);

    long double new_top_real = centre_real - factor*real_width/2;
    long double new_top_imag = centre_imag - factor*imag_height/2;
    long double new_bottom_real = centre_real + factor*real_width/2;
    long double new_bottom_imag = centre_imag + factor*imag_height/2;

    *viewport_top = CMPLXL(new_top_real, new_top_imag);
    *viewport_bottom = CMPLXL(new_bottom_real, new_bottom_imag);
}
