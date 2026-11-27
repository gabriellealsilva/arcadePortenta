#include "SdlDisplay.h"

#include <cstring>

namespace cps1
{

bool SdlDisplay::init(int width, int height, int scale, bool hidden)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return false;
    }
    sdlInit_ = true;

    window_ = SDL_CreateWindow("CPS-1 Emulator", width * scale, height * scale,
                               SDL_WINDOW_RESIZABLE | (hidden ? SDL_WINDOW_HIDDEN : 0));
    if (!window_)
    {
        SDL_Log("SDL_CreateWindow: %s", SDL_GetError());
        return false;
    }

    renderer_ = SDL_CreateRenderer(window_, nullptr);
    if (!renderer_)
    {
        SDL_Log("SDL_CreateRenderer: %s", SDL_GetError());
        return false;
    }

    SDL_SetRenderLogicalPresentation(renderer_, width, height,
                                     SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);

    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
                                 width, height);
    if (!texture_)
    {
        SDL_Log("SDL_CreateTexture: %s", SDL_GetError());
        return false;
    }

    void* pixels = nullptr;
    int   pitch  = 0;
    if (SDL_LockTexture(texture_, nullptr, &pixels, &pitch))
    {
        std::memset(pixels, 0, static_cast<std::size_t>(pitch) * static_cast<std::size_t>(height));
        SDL_UnlockTexture(texture_);
    }
    return true;
}

SdlDisplay::~SdlDisplay()
{
    if (texture_)
        SDL_DestroyTexture(texture_);
    if (renderer_)
        SDL_DestroyRenderer(renderer_);
    if (window_)
        SDL_DestroyWindow(window_);
    if (sdlInit_)
        SDL_Quit();
}

void SdlDisplay::beginFrame() const
{
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
}

void SdlDisplay::drawFramebuffer(std::span<const uint16_t> rgb565) const
{
    void* pixels = nullptr;
    int   pitch  = 0;
    if (!SDL_LockTexture(texture_, nullptr, &pixels, &pitch))
        return;
    std::memcpy(pixels, rgb565.data(), rgb565.size_bytes());
    SDL_UnlockTexture(texture_);
    SDL_RenderTexture(renderer_, texture_, nullptr, nullptr);
}

} // namespace cps1
