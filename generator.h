#ifndef _GENERATOR_H_
#define _GENERATOR_H_

extern const int GENERATED_SIZE;

/* Generates an artifact and sets its pixels in the given n by n array,
 * where n is defined by GENERATED_SIZE.
 *
 * pixels - the array of pixels to generate in
 * id - the unique id (seed) of the artifact to generate
 * alpha_mask - the alpha_mask for pixels on this system (e.g. 0xFF000000)
 */
void generate_artifact(int *pixels, int id, int alpha_mask);

#endif
