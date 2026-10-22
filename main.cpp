#include <SDL3/SDL_main.h>

#include <cstdio>
#include <cstring>

#include "gif_displayer.h"
#include "gif_loader.h"

int main(int argc, char* argv[])
{
    // Usage: cat_gif [configFile]
    GifConfig config;
    if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        printf("Usage: cat_gif [CONFIG_FILE]\n");
        printf("CONFIG_FILE is a text file with a path on the first and second line\n");
        printf("First line is 'new' dir and second line is 'used' dir for the gifs source\n");
        printf("\n");
        printf("If no config file specified, the below defaults are used:\n");
        printf("default CONFIG_FILE: %s\ndefault NEW_DIR: %s\ndefault USED_DIR: %s\n",
               config.configFile.c_str(), config.newDir.c_str(), config.usedDir.c_str());
        return 0;
    }

    if (argc > 1) {
        config.configFile = argv[1];
    }
    loadConfigFile(config);

    GifLoader loader(config);
    GifDisplayer displayer(loader);
    if (!displayer.init()) {
        return 1;
    }
    displayer.run();
    return 0;
}
