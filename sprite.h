#ifndef _SPRITE_H_
#define _SPRITE_H_

#include <SDL.h>

extern const int SPRITE_SIZE;

typedef struct Sprite {
    SDL_Texture *texture;
    SDL_Surface *surface;
} Sprite;

void sprite_gen(Sprite *sprite, SDL_Renderer *renderer);
void sprite_render(Sprite *sprite, int x, int y, SDL_Renderer *renderer);

#endif
