#pragma once
#include <SDL3/SDL.h>
#include <cstdint>
#include <span>

namespace cps1
{

/// @brief RAII owner of the SDL window, renderer and streaming framebuffer texture.
/// Declare it before anything that holds its renderer (e.g. BootloaderMenu).
class SdlDisplay
{
public:
    SdlDisplay() = default;
    ~SdlDisplay();

    SdlDisplay(const SdlDisplay&)            = delete;
    SdlDisplay& operator=(const SdlDisplay&) = delete;

    /// @brief Create the window at @p scale x the logical size.
    /// @return @c false (error already logged) on any SDL failure.
    bool init(int width, int height, int scale);

    SDL_Renderer* renderer() const { return renderer_; }
    void setTitle(const char* title) const { SDL_SetWindowTitle(window_, title); }

    /// @brief Clear the back buffer to black.
    void beginFrame() const;
    /// @brief Upload and draw an RGB565 framebuffer of the logical size.
    void drawFramebuffer(std::span<const uint16_t> rgb565) const;
    void endFrame() const { SDL_RenderPresent(renderer_); }

private:
    SDL_Window*   window_   = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture*  texture_  = nullptr;
    bool          sdlInit_  = false;
};

} // namespace cps1
