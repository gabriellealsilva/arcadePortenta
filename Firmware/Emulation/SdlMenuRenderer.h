#pragma once
#ifndef SDL_MAIN_HANDLED
#  define SDL_MAIN_HANDLED
#endif
#include <SDL3/SDL.h>
#include "UI/BootloaderMenu.h"

namespace cps1
{

/// @brief Host-emulator @ref MenuRenderer backend: draws @ref BootloaderMenu
/// via SDL's built-in 8 x 8 debug font.
class SdlMenuRenderer
{
public:
    /// @param renderer Must remain valid for the lifetime of this object.
    explicit SdlMenuRenderer(SDL_Renderer* renderer) : renderer(renderer) {}

    void clear(Color c)
    {
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        SDL_RenderClear(renderer);
    }

    void setColor(Color c) { SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a); }

    void fillRect(float x, float y, float w, float h)
    {
        const SDL_FRect rect = { x, y, w, h };
        SDL_RenderFillRect(renderer, &rect);
    }

    void drawText(float x, float y, const char* text)
    {
        SDL_RenderDebugText(renderer, x, y, text);
    }

private:
    SDL_Renderer* renderer;
};

static_assert(MenuRenderer<SdlMenuRenderer>);

} // namespace cps1
