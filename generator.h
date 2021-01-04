/* generator.c - generates artifacts
    author: Andrew Klinge
*/

#ifndef _GENERATOR_H_
#define _GENERATOR_H_

/* width and height in pixels of the generated artifacts. */
extern const int GENERATED_SIZE;

/* Generates an artifact into the given pixel array.
 *
 * pixels - the array of pixels to generate in (must be n x n, where 
 *      n = GENERATED_SIZE).
 * id - the unique id (seed) of the artifact to generate
 */
void generate_artifact(int *pixels, int id);

#endif
