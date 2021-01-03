#include <stdlib.h>

#include "sprite.h"

const int SPRITE_SIZE = 8;

static int Y(int y) {
    return SPRITE_SIZE * y;
}

static void transform(int *pix, int wdiv, int hdiv, int xt, int yt) {
    for (int x = 0; x < SPRITE_SIZE / wdiv; x++) {
        for (int y = 0; y < SPRITE_SIZE / hdiv; y++) {
            int rx = (xt == 0 ? x : xt - x);
            int ry = (yt == 0 ? y : yt - y);
            pix[rx + Y(ry)] = pix[x + Y(y)];
        }
    }
}

static void rotate(int *x, int *y) {
    int nx = SPRITE_SIZE - 1 - (*y);
    int ny = (*x);
    *x = nx;
    *y = ny;
}

void sprite_gen(Sprite *sprite, SDL_Renderer *renderer) {
    int32_t rmask, gmask, bmask, amask;
    #if SDL_BYTEORDER != SDL_BIG_ENDIAN
        rmask = 0xff000000;
        gmask = 0x00ff0000;
        bmask = 0x0000ff00;
        amask = 0x000000ff;
    #else
        rmask = 0x000000ff;
        gmask = 0x0000ff00;
        bmask = 0x00ff0000;
        amask = 0xff000000;
    #endif

    SDL_Surface *surface = SDL_CreateRGBSurface(0, SPRITE_SIZE, SPRITE_SIZE,
        32, rmask, gmask, bmask, amask);
    int32_t *pix = (int32_t*) surface->pixels;

    // randomly generate image
    for (int x = 0; x < SPRITE_SIZE; x++) {
        for (int y = 0; y < SPRITE_SIZE; y++) {
            if ((rand() & 3) <= 1) pix[x + Y(y)] = 1;
            else pix[x + Y(y)] = 0;
        }
    }

    // randomly assign two colors
    int col1 = rand() | amask, col2 = rand() | amask;
    for (int x = 0; x < SPRITE_SIZE; x++) {
        for (int y = 0; y < SPRITE_SIZE; y++) {
            if (pix[x + Y(y)] != 0) {
                if (rand() & 1) pix[x + Y(y)] = col1;
                else pix[x + Y(y)] = col2; 
            }
        }
    }
    
    // determine symmetry (rotate or reflect some quadrant(s) of sprite)
    int r = (rand() & 3);
    int r2 = (rand() & 1);
    const int ss = SPRITE_SIZE - 1;
    switch (r) {
        case 0: // vertical symmetry ||
            if (r2) { // rotate
                transform(pix, 2, 1, ss, ss);
            } else { // reflect
                transform(pix, 2, 1, ss, 0);
            }
            break;
        case 1: // horizontal symmetry =
            if (r2) { // rotate
                transform(pix, 1, 2, ss, ss);
            } else { // reflect
                transform(pix, 1, 2, 0, ss);
            }
            break;
        case 2: // quadrant symmetry ::
            if (r2) { // rotate
                const int half = SPRITE_SIZE / 2;
                for (int x = 0; x < half; x++) {
                    for (int y = 0; y < half; y++) {
                        int p = pix[x + Y(y)];
                        int rx = x;
                        int ry = y;

                        rotate(&rx, &ry);
                        pix[rx + Y(ry)] = p;

                        rotate(&rx, &ry);
                        pix[rx + Y(ry)] = p;

                        rotate(&rx, &ry);
                        pix[rx + Y(ry)] = p;
                    }
                }
            } else { // reflect
                transform(pix, 2, 2, ss, ss);
                transform(pix, 2, 2, 0, ss);
                transform(pix, 2, 2, ss, 0);
            }
            break;
        case 3: // diagonal symmetry %
            if (r2) { // forward '/'
                for (int x = 0; x < SPRITE_SIZE; x++) {
                    for (int y = 0; y < SPRITE_SIZE; y++) {
                        pix[x + Y(y)] = pix[ss - y + Y(ss - x)];
                    }
                }
            } else { // backward '\'
                for (int x = 0; x < SPRITE_SIZE; x++) {
                    for (int y = 0; y < SPRITE_SIZE; y++) {
                        pix[x + Y(y)] = pix[y + Y(x)];
                    }
                }
            }
            break;
    }

    sprite->surface = surface;
    sprite->texture = SDL_CreateTextureFromSurface(renderer, surface);
}
