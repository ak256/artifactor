# Artifactor

An "artifact" generator. Artifacts are randomly-generated, symmetric, two-color images. Due to their randomly-generated nature, most of them aren't that interesting, but occasionally you can discover some good ones. Here is an example of the generator in action:

<p align="center"><img src="screenshot.png"/></p>

Idea taken from the game Tea Garden by Younès Rabii (<a href="https://github.com/Pyrofoux">"Pyrofoux"</a>), where such artifacts are flowers that the player can find. This implementation is not a game, only a generator (with optional simple renderer using SDL2). You can browse artifacts, find the same ones again using their unique IDs, and save them as images.

Generator GUI controls:

* Arrow keys or WASD to navigate selection cursor to browse
* X to save selected artifact to image
* Number keys (0-9) to input specific ID to jump to
* Backspace to delete last entered number
* Enter to confirm ID input and to jump to the corresponding artifact 

The possible artifact IDs range from 0 to 4294967295 (2^32 - 1).
