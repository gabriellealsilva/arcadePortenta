#include "Snapshot.h"

#include <SDL3/SDL.h>

namespace cps1
{

bool savePng(std::span<const uint16_t> rgb565, int width, int height, const char* path)
{
    // SDL wants a mutable pointer but only reads from it here.
    SDL_Surface* surf = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGB565,
                                              const_cast<uint16_t*>(rgb565.data()), width * 2);
    const bool ok = surf && SDL_SavePNG(surf, path);
    SDL_DestroySurface(surf);
    return ok;
}

} // namespace cps1
