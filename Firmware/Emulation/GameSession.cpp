#include "GameSession.h"

#include <SDL3/SDL.h>
#include <cstdio>

#include "Log.h"
#include "core/CpsBConfig.h"
#include "RomAssembler.h"

namespace cps1
{

bool GameSession::start(FilesystemRomSource& src, const GameDescriptor& game)
{
    LOG("[EMU] Loading: %s", game.title);

    // Destroy any previous system before mutating the buffers it may still point at.
    system_.reset();
    frames_ = 0;

    if (!assembleRomImages(src, game, progRom_, gfxRom_))
    {
        SDL_Log("[EMU] Failed to assemble ROM image for '%s'", game.title);
        return false;
    }

    const CpsBConfig* cpsb = findCpsBConfig(game.id);
    if (!cpsb)
    {
        SDL_Log("[EMU] No CPS-B configuration for '%s'", game.id);
        return false;
    }

    system_ = std::make_unique<Cps1System>();
    if (!system_->load(progRom_, gfxRom_, *cpsb))
    {
        SDL_Log("[EMU] Failed to load '%s'", game.title);
        system_.reset();
        return false;
    }
    return true;
}

void GameSession::tick(bool trace)
{
    system_->tick();
    ++frames_;
    if (trace && frames_ % 60 == 0)
        std::fprintf(stderr, "[TRACE] frame %d PC=0x%06X\n", frames_, system_->programCounter());
}

const GameDescriptor* findGame(std::span<const GameDescriptor> games, std::string_view id)
{
    for (const auto& g : games)
        if (std::string_view(g.id) == id)
            return &g;
    return nullptr;
}

} // namespace cps1
