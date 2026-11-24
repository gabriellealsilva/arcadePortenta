#pragma once
#include <cstdint>
#include <span>

namespace cps1
{

/// @brief Save an RGB565 framebuffer as a PNG file.
/// @return @c false (error text via SDL_GetError) if the file could not be written.
bool savePng(std::span<const uint16_t> rgb565, int width, int height, const char* path);

} // namespace cps1
