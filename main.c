/* Main.c
 *
 * "Artifactor" game inspired by Roadside Picnic and Younes 
 * Rabii's game "Tea Garden"
 *
 * Got SDL setup thanks to:
 * https://www.caveofprogramming.com/guest-posts/drawing-an-image-sdl.html
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <sys/time.h>

#include "helper.h"
#include "sprite.h"

const int WINDOW_SIZE = 480;
const int MAP_SIZE = 32;
const int TEXTURE_MAP_SIZE = 4;
#define TEXTURE_COUNT (TEXTURE_MAP_SIZE * TEXTURE_MAP_SIZE)

static TTF_Font *font;
static SDL_Renderer *renderer;
static SDL_Texture **textures;
static int8_t *tiles;
static Sprite *sprite;
static int camx = 0, camy = 0;

static void init() {
    // generate sprites
    sprite = malloc(sizeof(Sprite));
    sprite_gen(sprite, renderer);

    // parse textures
    IMG_Init(IMG_INIT_PNG);
    textures = malloc(sizeof(SDL_Texture*) * TEXTURE_COUNT);
    SDL_Surface *teximg = IMG_Load("textures.png");
    SDL_Rect dstrect = {.x = 0, .y = 0, .w = SPRITE_SIZE, .h = SPRITE_SIZE};
    for (int i = 0; i < TEXTURE_COUNT; i++) {
        SDL_Rect rect;
        rect.x = (i % TEXTURE_MAP_SIZE) * SPRITE_SIZE + camx;
        rect.y = (i / TEXTURE_MAP_SIZE) * SPRITE_SIZE + camy;
        rect.w = SPRITE_SIZE;
        rect.h = SPRITE_SIZE;
        SDL_Surface *surface = SDL_CreateRGBSurface(0, rect.w, rect.h, 
            32, 0, 0, 0, 0);
        SDL_BlitSurface(teximg, &rect, surface, &dstrect);
        textures[i] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }
    SDL_FreeSurface(teximg);

    // create map
    tiles = malloc(MAP_SIZE * MAP_SIZE * 2);
    for (int x = 0; x < MAP_SIZE; x++) {
        for (int y = 0; y < MAP_SIZE; y++) {
            tiles[x * 2 + y * MAP_SIZE] = 5;
            tiles[x * 2 + 1 + y * MAP_SIZE] = 0;
        }
    }
}

static int spri = 0;
static bool update() {
    camx = (camx + 1) % SPRITE_SIZE;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_QUIT:
                return 1;
            case SDL_KEYDOWN: {
                switch (event.key.keysym.sym) {
                    case SDLK_UP: {
                        // save sprite to file
                        char buf[32];
                        snprintf(buf, 32, "%d.bmp", spri);
                        if (access(buf, F_OK) != 0) 
                            SDL_SaveBMP(sprite->surface, buf);
                        break;
                    }
                    case SDLK_DOWN: {
                        spri++;
                        SDL_FreeSurface(sprite->surface);
                        SDL_DestroyTexture(sprite->texture);
                        sprite_gen(sprite, renderer);
                        break;
                    }
                }
                break;
            }
        }
    }
    return 0;
}

static void render() {
    // init
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    int scale = 3;
    SDL_RenderSetScale(renderer, scale, scale);

    // render stuff
    for (int x = 0; x < MAP_SIZE; x++) {
        for (int y = 0; y < MAP_SIZE; y++) {
            int8_t tile_id = tiles[x * 2 + y * MAP_SIZE];
            if (tile_id == 0) continue;
            
            SDL_Texture *texture = textures[tile_id - 1];
            SDL_Rect dstrect;
            dstrect.x = x * SPRITE_SIZE + camx;
            dstrect.y = y * SPRITE_SIZE + camy;
            dstrect.w = SPRITE_SIZE;
            dstrect.h = SPRITE_SIZE;

            if (SDL_RenderCopy(renderer, texture, NULL, &dstrect)) {
                fprintf(stderr, "rendercopy failed: %s\n", SDL_GetError());
                exit(EXIT_FAILURE);
            }
        }
    }
    // browse artifacts
    SDL_Rect rect;
    rect.x = WINDOW_SIZE / scale / 2 - WINDOW_SIZE / scale / 8;
    rect.y = rect.x;
    rect.w = WINDOW_SIZE / scale / 4;
    rect.h = rect.w;
    SDL_RenderCopy(renderer, sprite->texture, NULL, &rect);

    // draw to screen
    SDL_RenderSetScale(renderer, 1, 1);
    SDL_RenderPresent(renderer);
}

/*static void draw_text(char *string, SDL_Color color) {
    SDL_Surface *surface = TTF_RenderText_Solid(font, string, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    
    SDL_Rect rect = {.x = 0, .y = 100, .w = 50, .h = 50};
    SDL_RenderCopy(renderer, texture, NULL, &rect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}*/

int main(int argc, char **argv) {
    // init window
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window* window = SDL_CreateWindow("Artifactor", SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, WINDOW_SIZE, WINDOW_SIZE, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Init();
    SDL_Event input;
    
    // init content
    font = TTF_OpenFont("/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf", 24);
    if (font == NULL) {
        fprintf(stderr, "not found\n");
        exit(EXIT_FAILURE);
    }
    init();

    // run
    while (true) {
        if (update()) break;

        static int last_tick = 0;
        if (SDL_GetTicks() - last_tick >= 1000 / 60) {
            render();
            last_tick = SDL_GetTicks();
        }

        // count frames
        SDL_Delay(1); // 60 ups/fps
    }

    // cleanup content
    TTF_CloseFont(font);

    // cleanup window
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
