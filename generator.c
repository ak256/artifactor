/* generator.c - generates two-color, symmetric, 8x8 artifacts
    author: Andrew Klinge
*/

#include <stdlib.h>

#include "generator.h"

const int GENERATED_SIZE = 8;

/* converts y-coordinate into index used in linear pixels array. */
static inline int Y(int y) {
    return GENERATED_SIZE * y;
}

/* will rotate and or reflect the image based on arguments.
 * 
 * pixels - pixels of the artifact's image
 * wdiv - divisor of image's width, specifies width of copied area
 * hdiv - divisor of image's height, specifies height of copied area
 * xt - x offset for dest pixels of copied area. (>= 0, reflects if != 0) 
 * yt - y offset for dest pixels of copied area (>= 0, reflects if != 0)
 */
static void transform(int *pixels, int wdiv, int hdiv, int xt, int yt) {
    for (int x = 0; x < GENERATED_SIZE / wdiv; x++) {
        for (int y = 0; y < GENERATED_SIZE / hdiv; y++) {
            int rx = (xt == 0 ? x : xt - x);
            int ry = (yt == 0 ? y : yt - y);
            pixels[rx + Y(ry)] = pixels[x + Y(y)];
        }
    }
}

/* rotates the point 90 degrees clockwise. */
static void rotate90(int *x, int *y) {
    int nx = GENERATED_SIZE - 1 - (*y);
    int ny = (*x);
    *x = nx;
    *y = ny;
}

void generate_artifact(int *pixels, int id) {
    srand(id);

    // randomly fill in pixels (0->transparent, 1->placeholder pixel)
    for (int x = 0; x < GENERATED_SIZE; x++) {
        for (int y = 0; y < GENERATED_SIZE; y++) {
            if ((rand() & 3) <= 1) pixels[x + Y(y)] = 1;
            else pixels[x + Y(y)] = 0;
        }
    }

    // randomly assign two colors
    int col1 = rand();
    int col2 = rand();
    for (int x = 0; x < GENERATED_SIZE; x++) {
        for (int y = 0; y < GENERATED_SIZE; y++) {
            if (pixels[x + Y(y)] != 0) {
                if (rand() & 1) pixels[x + Y(y)] = col1;
                else pixels[x + Y(y)] = col2; 
            }
        }
    }
    
    // determine symmetry (rotate or reflect some quadrant(s) of sprite)
    int sym_type = (rand() & 3);
    int rot_vs_ref = (rand() & 1); 
    const int reflect = GENERATED_SIZE - 1;
    switch (sym_type) {
        case 0: // vertical symmetry ||
            if (rot_vs_ref) { // rotate
                transform(pixels, 2, 1, reflect, reflect);
            } else { // reflect
                transform(pixels, 2, 1, reflect, 0);
            }
            break;
        case 1: // horizontal symmetry =
            if (rot_vs_ref) { // rotate
                transform(pixels, 1, 2, reflect, reflect);
            } else { // reflect
                transform(pixels, 1, 2, 0, reflect);
            }
            break;
        case 2: // quadrant symmetry ::
            if (rot_vs_ref) { // rotate
                const int half = GENERATED_SIZE / 2;
                for (int x = 0; x < half; x++) {
                    for (int y = 0; y < half; y++) {
                        int p = pixels[x + Y(y)];
                        int rx = x;
                        int ry = y;

                        rotate90(&rx, &ry);
                        pixels[rx + Y(ry)] = p;

                        rotate90(&rx, &ry);
                        pixels[rx + Y(ry)] = p;

                        rotate90(&rx, &ry);
                        pixels[rx + Y(ry)] = p;
                    }
                }
            } else { // reflect
                transform(pixels, 2, 2, reflect, reflect);
                transform(pixels, 2, 2, 0, reflect);
                transform(pixels, 2, 2, reflect, 0);
            }
            break;
        case 3: // diagonal symmetry %
            if (rot_vs_ref) { // forward '/'
                for (int x = 0; x < GENERATED_SIZE; x++) {
                    for (int y = 0; y < GENERATED_SIZE; y++) {
                        pixels[x + Y(y)] = pixels[reflect - y + Y(reflect - x)];
                    }
                }
            } else { // backward '\'
                for (int x = 0; x < GENERATED_SIZE; x++) {
                    for (int y = 0; y < GENERATED_SIZE; y++) {
                        pixels[x + Y(y)] = pixels[y + Y(x)];
                    }
                }
            }
            break;
    }
}
