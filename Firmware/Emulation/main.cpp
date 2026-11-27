#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <cstdio>
#include <filesystem>
#include <optional>

#include "CommandLine.h"
#include "FilesystemRomSource.h"
#include "GameSession.h"
#include "Log.h"
#include "SdlDisplay.h"
#include "SdlMenuRenderer.h"
#include "Snapshot.h"
#include "UI/BootloaderMenu.h"

/// @brief CPS-1 native resolution (Street Fighter II reference).
static constexpr int CPS1_WIDTH = 384;
static constexpr int CPS1_HEIGHT = 224;
static constexpr int WINDOW_SCALE = 3;

/// @brief Headless run: emulate N frames, save a PNG, return the exit code.
static int runSnapshot(cps1::GameSession &session, const cps1::Options &opt)
{
    for (int frame = 0; frame < opt.frames; ++frame)
        session.tick(opt.trace);

    if (!cps1::savePng(session.framebuffer(), CPS1_WIDTH, CPS1_HEIGHT, opt.snapshotPath.c_str()))
    {
        std::fprintf(stderr, "[EMU] Snapshot failed: %s\n", SDL_GetError());
        return 1;
    }
    std::fprintf(stderr, "[EMU] Saved %s after %d frames, PC=0x%06X\n", opt.snapshotPath.c_str(),
                 opt.frames, session.programCounter());
    return 0;
}

int main(int argc, char *argv[])
{
    // ROMS_PATH is the absolute path baked in at configure time by CMake;
    // a positional argument overrides it.
    cps1::Options opt;
    opt.romsPath = ROMS_PATH;
    if (!cps1::parseCommandLine(argc, argv, opt))
        return 2;

    {
        std::error_code ec;
        if (!std::filesystem::is_directory(opt.romsPath, ec))
        {
            std::fprintf(stderr,
                         "[ROM] ROMS directory not found: %s\n"
                         "[ROM] Either pass the path as argv[1], or ensure ROMS/ exists at:\n"
                         "[ROM]   " ROMS_PATH "\n",
                         opt.romsPath.string().c_str());
            return 1;
        }
    }

    cps1::SdlDisplay display;
    if (!display.init(CPS1_WIDTH, CPS1_HEIGHT, WINDOW_SCALE, opt.headless()))
        return 1;

    LOG("[ROM] Using ROMS path: %s", opt.romsPath.string().c_str());
    cps1::FilesystemRomSource romSource(opt.romsPath);
    romSource.scan();

    // Declared after `display`: the menu renderer holds its SDL renderer.
    cps1::SdlMenuRenderer menuRenderer(display.renderer());
    cps1::BootloaderMenu<cps1::SdlMenuRenderer> menu(menuRenderer);
    menu.setGames(romSource.availableGames());

    cps1::GameSession session;

    // --game: boot straight into a title, skipping the menu.
    if (!opt.gameId.empty())
    {
        const auto *game = cps1::findGame(romSource.availableGames(), opt.gameId);
        if (!game || !session.start(romSource, *game))
        {
            std::fprintf(stderr, "[EMU] Cannot start game '%s'\n", opt.gameId.c_str());
            return 1;
        }
        display.setTitle(game->title);
    }

    if (opt.headless())
        return runSnapshot(session, opt);

    bool running = true;
    SDL_Event event;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
                break;
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE &&
                !session.active())
            {
                running = false;
                break;
            }

            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE &&
                session.active())
            {
                session.stop();
                display.setTitle("CPS-1 Emulator");
                continue;
            }

            if (!session.active())
            {
                std::optional<cps1::MenuInput> input;
                if (event.type == SDL_EVENT_KEY_DOWN)
                {
                    switch (event.key.key)
                    {
                        case SDLK_UP:     input = cps1::MenuInput::Up; break;
                        case SDLK_DOWN:   input = cps1::MenuInput::Down; break;
                        case SDLK_RETURN:
                        case SDLK_KP_ENTER:
                            input = cps1::MenuInput::Confirm;
                            break;
                        default: break;
                    }
                }

                if (auto sel = input ? menu.handleInput(*input) : std::nullopt)
                {
                    if (session.start(romSource, *sel))
                        display.setTitle(sel->title);
                }
            }
        }

        display.beginFrame();

        if (!session.active())
        {
            menu.render();
        }
        else
        {
            session.tick(opt.trace);
            display.drawFramebuffer(session.framebuffer());
        }

        display.endFrame();
    }

    return 0;
}
