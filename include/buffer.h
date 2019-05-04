#ifndef BUFFER_H_MATTONI
#define BUFFER_H_MATTONI

#include <stdlib.h>
#include <SDL2/SDL.h>

struct buffer_t;

struct buffer_t *make_buffer(size_t screen_w, size_t screen_h);
void free_buffer(struct buffer_t **buf);
void set_color(struct buffer_t *buf, unsigned int x, unsigned int y, SDL_Color color);

#endif // BUFFER_H_MATTONI

