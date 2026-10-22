#include "gif_displayer.h"

#include <algorithm>

namespace fs = std::filesystem;

namespace {

constexpr const char* kStorageMessage = "Misconfigured, empty, or missing new/ or used/ dir";
constexpr const char* kCriticalMessage =
    "Critical: misconfigured, missing or empty used/new dirs, and fallback GIF missing or corrupted";

}  // namespace

GifDisplayer::GifDisplayer(GifLoader& loader) : loader_(loader) {}

GifDisplayer::~GifDisplayer()
{
    unloadGif();
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
    }
    if (window_) {
        SDL_DestroyWindow(window_);
    }
    SDL_Quit();
}

bool GifDisplayer::init()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return false;
    }
    if (!SDL_CreateWindowAndRenderer("cat-gif", 800, 600, SDL_WINDOW_FULLSCREEN,
                                     &window_, &renderer_)) {
        SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
        return false;
    }
    SDL_SetRenderVSync(renderer_, 1);
    SDL_HideCursor();
    return true;
}

void GifDisplayer::unloadGif()
{
    for (SDL_Texture* t : frames_) {
        SDL_DestroyTexture(t);
    }
    frames_.clear();
    if (anim_) {
        IMG_FreeAnimation(anim_);
        anim_ = nullptr;
    }
    frameIndex_ = 0;
}

bool GifDisplayer::loadGif(const fs::path& path)
{
    unloadGif();

    anim_ = IMG_LoadAnimation(path.string().c_str());
    if (!anim_ || anim_->count <= 0) {
        SDL_Log("Couldn't load %s: %s", path.string().c_str(), SDL_GetError());
        unloadGif();
        return false;
    }

    for (int i = 0; i < anim_->count; ++i) {
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer_, anim_->frames[i]);
        if (!tex) {
            SDL_Log("Couldn't create texture: %s", SDL_GetError());
            unloadGif();
            return false;
        }
        frames_.push_back(tex);
    }
    frameIndex_ = 0;
    nextFrameAt_ = SDL_GetTicks() + std::max(anim_->delays[0], 20);
    return true;
}

// (Re)loads whatever the loader currently offers, falling back to the fallback GIF.
void GifDisplayer::reload()
{
    critical_ = true;
    if (auto path = loader_.currentGif()) {
        if (loadGif(*path)) {
            critical_ = false;
            return;
        }
        // The local copy may be corrupt; try the fallback.
        if (*path != loader_.config().fallbackGif && loadGif(loader_.config().fallbackGif)) {
            critical_ = false;
            return;
        }
    }
    unloadGif();
}

void GifDisplayer::advanceAnimation(uint64_t nowMs)
{
    if (!anim_ || anim_->count < 2) {
        return;
    }
    // Catch up if we fell behind, but never loop more than once per call.
    if (nowMs >= nextFrameAt_) {
        frameIndex_ = (frameIndex_ + 1) % anim_->count;
        nextFrameAt_ = nowMs + std::max(anim_->delays[frameIndex_], 20);
    }
}

void GifDisplayer::drawBanner(const std::string& message) const
{
    int w = 0, h = 0;
    SDL_GetCurrentRenderOutputSize(renderer_, &w, &h);

    const float charSize = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;
    const float textW = charSize * static_cast<float>(message.size());
    const float scale = std::min(3.0f, 0.9f * static_cast<float>(w) / textW);
    const float pad = 8.0f;

    // Work in scaled coordinates so the debug font is enlarged.
    SDL_SetRenderScale(renderer_, scale, scale);
    const float sw = static_cast<float>(w) / scale;
    const float sh = static_cast<float>(h) / scale;
    SDL_FRect box{(sw - textW) / 2 - pad, (sh - charSize) / 2 - pad, textW + 2 * pad,
                  charSize + 2 * pad};

    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer_, &box);
    SDL_SetRenderDrawColor(renderer_, 255, 0, 0, 255);
    SDL_RenderDebugText(renderer_, (sw - textW) / 2, (sh - charSize) / 2, message.c_str());
    SDL_SetRenderScale(renderer_, 1.0f, 1.0f);
}

void GifDisplayer::render()
{
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);

    if (!frames_.empty()) {
        int w = 0, h = 0;
        SDL_GetCurrentRenderOutputSize(renderer_, &w, &h);
        // Aspect-fit, centered.
        const float scale = std::min(static_cast<float>(w) / static_cast<float>(anim_->w),
                                     static_cast<float>(h) / static_cast<float>(anim_->h));
        const float dw = static_cast<float>(anim_->w) * scale;
        const float dh = static_cast<float>(anim_->h) * scale;
        SDL_FRect dst{(static_cast<float>(w) - dw) / 2, (static_cast<float>(h) - dh) / 2, dw, dh};
        SDL_RenderTexture(renderer_, frames_[frameIndex_], nullptr, &dst);
    }

    if (critical_) {
        drawBanner(kCriticalMessage);
    } else if (loader_.status() == StorageStatus::StorageError) {
        drawBanner(kStorageMessage);
    }
    SDL_RenderPresent(renderer_);
}

void GifDisplayer::run()
{
    loader_.update();
    reload();

    running_ = true;
    uint64_t lastTick = SDL_GetTicks();

    while (running_) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
                running_ = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE) {
                    running_ = false;
                } else if (event.key.key == SDLK_R && !event.key.repeat && rHeldSince_ == 0) {
                    rHeldSince_ = SDL_GetTicks();
                    rFired_ = false;
                }
                break;
            case SDL_EVENT_KEY_UP:
                if (event.key.key == SDLK_R) {
                    rHeldSince_ = 0;
                }
                break;
            default:
                break;
            }
        }

        const uint64_t now = SDL_GetTicks();

        // Holding 'r' for 3 seconds forces a rotation (once per hold).
        if (rHeldSince_ != 0 && !rFired_ && now - rHeldSince_ >= kRotateHoldMs) {
            rFired_ = true;
            if (loader_.rotate()) {
                reload();
            }
        }

        // Periodic upkeep: config edits, 7AM rotation, USB retries, pending moves.
        if (now - lastTick >= kTickIntervalMs) {
            lastTick = now;
            const bool dirsChanged = loader_.reloadConfig();
            // Also retry loading when nothing is displayable, in case
            // the USB stick came back.
            if (loader_.update() || dirsChanged || critical_) {
                reload();
            }
        }

        advanceAnimation(now);
        render();
    }
}
