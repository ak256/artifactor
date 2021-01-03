/* browser.c - program to view and browse generated artifacts
    author: Andrew Klinge
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <SDL.h>
#include <SDL_image.h>
#include <stdbool.h>
#include "generator.h"

static const int WINDOW_WIDTH = 640;
static const int WINDOW_HEIGHT = 220;
static const int ARTIFACTS_IN_VIEW = 5;
#define center_i (ARTIFACTS_IN_VIEW / 2)

typedef struct Artifact {
    SDL_Texture *texture;
    SDL_Surface *surface;
} Artifact;

static SDL_Renderer *renderer;
static Artifact *artifacts;
static int off_i = 0;
static int offx = 0; // offset for rendering artifacts

/* generates a new Artifact (images for SDL usage). */
static Artifact generate_artifact_SDL(int id) {
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        const uint32_t R_MASK = 0xff000000;
        const uint32_t G_MASK = 0x00ff0000;
        const uint32_t B_MASK = 0x0000ff00;
        const uint32_t A_MASK = 0x000000ff;
    #else
        const uint32_t R_MASK = 0x000000ff;
        const uint32_t G_MASK = 0x0000ff00;
        const uint32_t B_MASK = 0x00ff0000;
        const uint32_t A_MASK = 0xff000000;
    #endif

    Artifact artifact;
    artifact.surface = SDL_CreateRGBSurface(0, GENERATED_SIZE, 
        GENERATED_SIZE, 32, R_MASK, G_MASK, B_MASK, A_MASK);
    generate_artifact((int*) artifact.surface->pixels, id, A_MASK);
    artifact.texture = SDL_CreateTextureFromSurface(renderer, artifact.surface);
    SDL_SetTextureBlendMode(artifact.texture, SDL_BLENDMODE_BLEND);
    return artifact;
}

/* frees resources of no-longer-needed artifact. */
static void free_artifact(Artifact *artifact) {
    SDL_FreeSurface(artifact->surface);
    SDL_DestroyTexture(artifact->texture);
}

/* initializes program. */
static void init() {
    // init artifacts in view
    artifacts = malloc(sizeof(Artifact) * ARTIFACTS_IN_VIEW);
    for (int i = 0; i < ARTIFACTS_IN_VIEW; i++) {
        artifacts[i] = generate_artifact_SDL(i);
    }
}

/* receive key input. returns 1 if program should end. */
static bool update() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_QUIT:
                return 1;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_SPACE:
                    case SDLK_DOWN: {
                        // save artifact to file
                        char buf[64];
                        int id = off_i + center_i;
                        snprintf(buf, 64, "%d.bmp", id);
                        if (access(buf, F_OK) != 0) // don't overwrite 
                            SDL_SaveBMP(artifacts[id].surface, buf);
                        break;
                    }
                    case SDLK_a:
                    case SDLK_LEFT: {
                        // browse leftwards
                        Artifact *at = &artifacts[ARTIFACTS_IN_VIEW - 1];
                        free_artifact(at);
                        //*at = generate_artifact_SDL(curr_
                        //for (int i = 
                        break;
                    }
                    case SDLK_d:
                    case SDLK_RIGHT: {
                        // browse rightwards
                        break;
                    }
                }
                break;
        }
    }
    return 0;
}

static void render() {
    // clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // draw artifacts
    SDL_Rect rect;
    rect.w = WINDOW_WIDTH / 4;
    rect.h = rect.w;
    int cx = WINDOW_WIDTH / 2 - WINDOW_WIDTH / 8;
    rect.y = 10;
    // draw back ones first (outer-most towards center)
    for (int i = 0; i < ARTIFACTS_IN_VIEW / 2; i++) {
        int dist = (ARTIFACTS_IN_VIEW / 2 - i);
        int alpha = 255 / (dist * 2) / 3;
        int width = rect.w + rect.w / 8;

        // left one
        SDL_Texture *tex = artifacts[i].texture;
        rect.x = cx - dist * width + offx;
        SDL_SetTextureAlphaMod(tex, alpha);
        SDL_RenderCopy(renderer, tex, NULL, &rect);

        // corresponding right one
        tex = artifacts[ARTIFACTS_IN_VIEW - 1 - i].texture;
        rect.x = cx + dist * width + offx;
        SDL_SetTextureAlphaMod(tex, alpha);
        SDL_RenderCopy(renderer, tex, NULL, &rect);
    }
    // draw center one
    rect.x = cx + offx;
    SDL_SetTextureAlphaMod(artifacts[center_i].texture, 255);
    SDL_RenderCopy(renderer, artifacts[center_i].texture, NULL, &rect);

    // draw to screen
    SDL_RenderPresent(renderer);
}

int main(int argc, char **argv) {
    // init
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window* window = SDL_CreateWindow("Artifactor", SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    init();

    // run
    while (true) {
        if (update()) break;

        // render ~60 fps
        static int last_tick = 0;
        if (SDL_GetTicks() - last_tick >= 1000 / 60) {
            render();
            last_tick = SDL_GetTicks();
        }
        SDL_Delay(1); // no reason to be constantly running. take a break
    }

    // cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
