#include <SDL3/SDL_main.h>

#include <cstdio>

#include "gif_displayer.h"
#include "gif_loader.h"

int main(int argc, char* argv[])
{
    // Usage: cat_gif [newDir] [usedDir]
    GifConfig config;
    if (argc > 1) {
        config.newDir = argv[1];
    }
    if (argc > 2) {
        config.usedDir = argv[2];
    }

    GifLoader loader(config);
    GifDisplayer displayer(loader);
    if (!displayer.init()) {
        return 1;
    }
    displayer.run();
    return 0;
}
