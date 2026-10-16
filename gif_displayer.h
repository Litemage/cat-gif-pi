#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "gif_loader.h"

// Owns the SDL window/renderer and the application's main loop.
//   Esc          quit
//   hold 'r' 3s  force a rotation to the next GIF
class GifDisplayer {
public:
    explicit GifDisplayer(GifLoader& loader);
    ~GifDisplayer();

    GifDisplayer(const GifDisplayer&) = delete;
    GifDisplayer& operator=(const GifDisplayer&) = delete;

    bool init();
    void run();

private:
    bool loadGif(const std::filesystem::path& path);
    void unloadGif();
    void reload();
    void advanceAnimation(uint64_t nowMs);
    void render();
    void drawBanner(const std::string& message) const;

    static constexpr uint64_t kRotateHoldMs = 3000;
    static constexpr uint64_t kTickIntervalMs = 5000;

    GifLoader& loader_;
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;

    IMG_Animation* anim_ = nullptr;
    std::vector<SDL_Texture*> frames_;
    int frameIndex_ = 0;
    uint64_t nextFrameAt_ = 0;

    bool critical_ = false;  // nothing displayable at all
    bool running_ = false;

    uint64_t rHeldSince_ = 0;  // 0 = not held
    bool rFired_ = false;
};
