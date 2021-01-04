/* browser.c - program to view and browse generated artifacts
    author: Andrew Klinge
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdbool.h>
#include <limits.h>
#include <errno.h>

#include "generator.h"

static const int WINDOW_WIDTH = 480;
static const int WINDOW_HEIGHT = 480;
static const int SCALE = 6; // factor to scale-up artifact images by
static const int FADE_TIME = 1000; // how long fading effect takes (ms)
static const int PRE_FADE_TIME = 3000; // time before fading begins (ms)
// max # of digits (+null) for uint string
#define MAX_DIGITS 11 

static const SDL_Color COLOR_WHITE = {.r=255, .g=255, .b=255, .a=255};
static const SDL_Color COLOR_RED = {.r=245, .g=0, .b=0, .a=255};
static const SDL_Color COLOR_BLACK = {.r=0, .g=0, .b=0, .a=255};
static const SDL_Color COLOR_GREEN = {.r=0, .g=245, .b=0, .a=255};

typedef struct Artifact {
    int id;
    SDL_Texture *texture;
    SDL_Surface *surface;
} Artifact;

static SDL_Renderer *renderer;
static TTF_Font *font, *font_small;
static Artifact *artifacts; // displayed artifacts buffer
static int size; // size of artifacts
static int cursorx, cursory; // artifact selection cursor on screen
static int rows, cols; // rows and columns of artifacts on screen at a time
static int onscreen_offset; // index into artifacts where onscreen ones begin
static int xoff, yoff; // rendering offset for artifacts grid
static char input[MAX_DIGITS]; // for input ID to jump to
static char selected[MAX_DIGITS]; // current ID
static int input_fade = -1; // text fade animation time started
static SDL_Color input_color; // text color

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

    Artifact artifact = {.id = id};
    artifact.surface = SDL_CreateRGBSurface(0, GENERATED_SIZE, GENERATED_SIZE, 
        32, R_MASK, G_MASK, B_MASK, A_MASK);
    generate_artifact((int*) artifact.surface->pixels, id);
    // apply alpha mask, ensure a=255
    for (int i = 0; i < GENERATED_SIZE * GENERATED_SIZE; i++) {
        ((int*) artifact.surface->pixels)[i] |= A_MASK;
    }
    artifact.texture = SDL_CreateTextureFromSurface(renderer, artifact.surface);
    return artifact;
}

/* frees resources of no-longer visible artifact. */
static void free_artifact(Artifact *artifact) {
    SDL_FreeSurface(artifact->surface);
    SDL_DestroyTexture(artifact->texture);
}

/* updates selected artifact id string. */
static void update_selected() {
    uint32_t id = cursorx + cursory * cols;
    snprintf(selected, MAX_DIGITS, "%u", id);
}

/* moves the displayed rows of artifacts down (false) or up (true). */
static void shift_rows(bool up_vs_down) {
    int first_id = artifacts[0].id;
    if (up_vs_down) {
        // remove shifted-out artifacts
        for (int i = 0; i < onscreen_offset; i++) {
            free_artifact(&artifacts[i]);
        }
        // shift up artifacts
        for (int i = 0; i < onscreen_offset + rows * cols; i++) {
            artifacts[i] = artifacts[i + cols];
        }
        // generate next artifacts offscreen
        for (int i = 0; i < onscreen_offset; i++) {
            int index = onscreen_offset + rows * cols + i;
            int id = first_id + index + onscreen_offset;
            artifacts[index] = generate_artifact_SDL(id);
        }
    } else {
        // remove shifted-out artifacts
        for (int i = 0; i < onscreen_offset; i++) {
            int index = i + onscreen_offset + rows * cols;
            free_artifact(&artifacts[index]);
        }
        // shift down artifacts
        for (int i = size - 1; i >= onscreen_offset; i--) {
            artifacts[i] = artifacts[i - cols];
        }
        // generate next artifacts offscreen
        for (int i = 0; i < onscreen_offset; i++) {
            int id = first_id - onscreen_offset + i;
            artifacts[i] = generate_artifact_SDL(id);
        }
    }
}

/* jumps directly to the artifact with the given id. */
static void jump(uint32_t id) {
    int row = id / cols;
    int col = id % cols;
    cursorx = col;
    if (cursory == row) return; // already here!
    cursory = row;
    update_selected();

    const int crow = onscreen_offset + cols * (cols / 2); // tiles to center row
    if (id < crow) {
        // appears in top edge of screen
        for (int i = onscreen_offset; i < size; i++) {
            free_artifact(&artifacts[i]);
            artifacts[i] = generate_artifact_SDL(i - onscreen_offset);
        }
    } else if (id >= UINT_MAX - crow) {
        // appears in bottom edge of screen
        for (int i = 0; i < size - onscreen_offset; i++) {
            free_artifact(&artifacts[i]);
            artifacts[i] = generate_artifact_SDL(i + UINT_MAX - size);
        }
    } else {
        // load in artifact view centered on target
        for (int i = 0; i < size; i++) {
            free_artifact(&artifacts[i]);
            artifacts[i] = generate_artifact_SDL(id + i - crow - col);
        }
    }
}

static void init() {
    // populate view with artifacts (+2 to allow for spacings)
    cols = (WINDOW_WIDTH / (GENERATED_SIZE * SCALE + 2));//
    rows = (WINDOW_HEIGHT / (GENERATED_SIZE * SCALE + 2));
    int offscreen = cols * 2;
    size = cols * rows + offscreen;
    artifacts = malloc(sizeof(Artifact) * size);
    onscreen_offset = offscreen / 2;
    xoff = (WINDOW_WIDTH - ((GENERATED_SIZE * SCALE + 1) * cols)) / 2;
    yoff = (WINDOW_HEIGHT - ((GENERATED_SIZE * SCALE + 1) * rows)) / 2;
    cursorx = 0;
    cursory = 0;
    update_selected();

    // generate artifacts
    for (int i = 0; i < size; i++) {
        artifacts[i] = generate_artifact_SDL(i - onscreen_offset);
    }

    // init font
    font = TTF_OpenFont("FreeMonoBold.ttf", 24);
    if (font == NULL) {
        fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
        exit(EXIT_FAILURE);
    }
    font_small = TTF_OpenFont("FreeMonoBold.ttf", 18);
}

/* receive key input. returns 1 if program should end. */
static bool update() {
    static const int input_size = 11; // 10 digits max (uint) + null byte
    static int input_i = 0; // index in input

    if (input_fade != -1 
    && SDL_GetTicks() - input_fade >= FADE_TIME + PRE_FADE_TIME) {
        input_fade = -1;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_QUIT:
                return 1;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_BACKSPACE: {
                        if (input_i > 0) {
                            input[input_i - 1] = '\0';
                            input_i--;
                            if (input_i == 0) input_fade = -1;
                        }
                        break;
                    }
                    case SDLK_0:
                    case SDLK_1:
                    case SDLK_2:
                    case SDLK_3:
                    case SDLK_4:
                    case SDLK_5:
                    case SDLK_6:
                    case SDLK_7:
                    case SDLK_8:
                    case SDLK_9: {
                        // input number as id to jump to
                        if (SDL_GetTicks() - input_fade > PRE_FADE_TIME) {
                            // reset input buffer for new number
                            input_i = 0;
                        }
                        if (input_i >= input_size) break;
                        input_color = COLOR_WHITE;
                        input_fade = SDL_GetTicks();
                        input[input_i] = event.key.keysym.sym;
                        input_i++;
                        input[input_i] = '\0';
                        break;
                    }
                    case SDLK_RETURN: {
                        if (input_i > 0) {
                            // parse number
                            errno = 0;
                            uint32_t id = (uint32_t) strtol(input, NULL, 10);
                            if (errno || id < 0 || id > UINT_MAX) {
                                // invalid number
                                input_color = COLOR_RED;
                            } else {
                                input_color = COLOR_GREEN;
                                jump(id);
                            }
                            input_fade = SDL_GetTicks() - PRE_FADE_TIME;
                        }
                        break;
                    }
                    case SDLK_x: {
                        // save artifact to file
                        uint32_t id = cursorx + cursory * cols;
                        char buf[MAX_DIGITS + 4];
                        snprintf(buf, MAX_DIGITS + 4, "%u.bmp", id);
                        // don't overwrite existing file
                        if (access(buf, F_OK) == 0) break;

                        int index = onscreen_offset + cursorx;
                        if (cursory <= rows / 2) {
                            index += (cursory % rows) * cols;
                        } else if(cursory >= UINT_MAX / rows - rows / 2) {
                            int middle = UINT_MAX / rows - rows / 2;
                            index += (cursory - middle + rows / 2) * cols;
                        } else {
                            index += (rows / 2) * cols;
                        }
                        SDL_SaveBMP(artifacts[index].surface, buf);
                        break;
                    }
                    case SDLK_d:
                    case SDLK_RIGHT: {
                        // move cursor right
                        cursorx++;
                        if (cursorx >= cols) {
                            if (cursory < UINT_MAX / rows) {
                                // wrap around to next row
                                cursorx = 0;
                                goto _MOVE_DOWN;
                            } else {
                                cursorx = cols - 1;
                            }
                        }
                        update_selected();
                        break;
                    }
                    case SDLK_a:
                    case SDLK_LEFT: {
                        // move cursor left
                        cursorx--;
                        if (cursorx < 0) {
                            if (cursory > 0) {
                                // wrap back to last row
                                cursorx = cols - 1;
                                goto _MOVE_UP;
                            } else {
                                cursorx = 0;
                            }
                        }
                        update_selected();
                        break;
                    }
                    case SDLK_s:
                    case SDLK_DOWN: {
                    _MOVE_DOWN:
                        // move cursor down
                        if (cursory < UINT_MAX / rows) {
                            cursory++;
                            // cursor stays center until at end of page
                            if (cursory > rows / 2
                            && cursory <= UINT_MAX / rows - rows / 2) 
                                shift_rows(true);
                        }
                        update_selected();
                        break;
                    }
                    case SDLK_w:
                    case SDLK_UP: {
                    _MOVE_UP:
                        // move cursor up
                        if (cursory > 0) {
                            cursory--;
                            // cursor stays center until at end of page
                            if (cursory >= rows / 2
                            && cursory < UINT_MAX / rows - rows / 2) 
                                shift_rows(false);
                        }
                        update_selected();
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
    rect.w = GENERATED_SIZE * SCALE;
    rect.h = rect.w;
    for (int i = 0; i < rows * cols; i++) {
        Artifact *at = &artifacts[i + onscreen_offset];
        rect.x = (i % cols) * (GENERATED_SIZE * SCALE + 1) + xoff;
        rect.y = (i / cols) * (GENERATED_SIZE * SCALE + 1) + yoff;
        SDL_RenderCopy(renderer, at->texture, NULL, &rect);
    }

    // draw cursor
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    rect.w = GENERATED_SIZE * SCALE + 1;
    rect.h = rect.w;
    rect.x = cursorx * rect.w + xoff;
    if (cursory >= rows / 2 && cursory <= UINT_MAX / rows - rows / 2) {
        // keep centered correctly until at edges of list
        rect.y = (rows / 2) * rect.h;
    } else {
        if (cursory < rows / 2) rect.y = cursory * rect.h;
        else rect.y = (rows/2 + cursory - (UINT_MAX/rows - rows/2)) * rect.h;
    }
    rect.y += yoff;
    SDL_RenderDrawRect(renderer, &rect);

    // draw input text
    if (input_fade != -1) {
        if (SDL_GetTicks() - input_fade > PRE_FADE_TIME + FADE_TIME) {
            input_color.a = 255 * (PRE_FADE_TIME + FADE_TIME 
                - (SDL_GetTicks() - input_fade)) / FADE_TIME;
        }
        SDL_Surface *surface = TTF_RenderText_Shaded(font, input, input_color,
            COLOR_BLACK);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        rect.w = surface->w;
        rect.h = surface->h;
        rect.x = (WINDOW_WIDTH - rect.w) / 2;
        rect.y = (WINDOW_HEIGHT - rect.h) / 2;
        SDL_RenderCopy(renderer, texture, NULL, &rect);

        // don't forget to free
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
    
    { // draw selected artifact id
        SDL_Surface *surface = TTF_RenderText_Solid(font_small, selected, 
            COLOR_WHITE);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        rect.w = surface->w;
        rect.h = surface->h;
        rect.x = 1;
        rect.y = 1;
        SDL_RenderCopy(renderer, texture, NULL, &rect);

        // don't forget to free
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

    // draw to screen
    SDL_RenderPresent(renderer);
}

int main(int argc, char **argv) {
    // init
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window* window = SDL_CreateWindow("Artifactor", SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Init();
    init();

    // run
    while (!update()) {
        // render ~60 fps
        static int last_tick = 0;
        if (SDL_GetTicks() - last_tick >= 1000 / 60) {
            render();
            last_tick = SDL_GetTicks();
        }
        SDL_Delay(1); // don't max out the CPU. don't need constant updates
    }

    // cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
