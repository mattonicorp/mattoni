#include <stdlib.h>

#include "buffer.h"

struct buffer_t *make_buffer(size_t screen_w, size_t screen_h) {
    
    struct buffer_t *buf = (struct buffer_t *)malloc(sizeof(struct buffer_t));
    buf->width = screen_w;
    buf->height = screen_h;
    buf->colors = (SDL_Color *)malloc(sizeof(SDL_Color) * screen_w * screen_h);

    return buf;
}

void free_buffer(struct buffer_t **buf) {
    free((*buf)->colors);
    free(*buf);
    *buf = NULL;
}

void set_color(struct buffer_t *buf, unsigned int x, unsigned int y, SDL_Color color) {
    buf->colors[x + y * buf->width] = color;
}

