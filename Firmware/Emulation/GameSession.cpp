#include "GameSession.h"

#include <SDL3/SDL.h>

#include "Log.h"
#include "RomAssembler.h"

namespace cps1
{

bool GameSession::start(FilesystemRomSource& src, const GameDescriptor& game)
{
    LOG("[EMU] Loading: %s", game.title);

    // Destroy any previous system before mutating the buffers it may still point at.
    system_.reset();

    if (!assembleRomImages(src, game, progRom_, gfxRom_))
    {
        SDL_Log("[EMU] Failed to assemble ROM image for '%s'", game.title);
        return false;
    }

    system_ = std::make_unique<Cps1System>();
    if (!system_->load(progRom_, gfxRom_))
    {
        SDL_Log("[EMU] Failed to load '%s'", game.title);
        system_.reset();
        return false;
    }
    return true;
}

} // namespace cps1
