#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

#include "FilesystemRomSource.h"
#include "Log.h"
#include "RomAssembler.h"
#include "SdlMenuRenderer.h"
#include "UI/BootloaderMenu.h"
#include "core/Cps1System.h"

/// @brief CPS-1 native resolution (Street Fighter II reference).
static constexpr int CPS1_WIDTH = 384;
static constexpr int CPS1_HEIGHT = 224;
static constexpr int WINDOW_SCALE = 3;

int main(int argc, char *argv[])
{
    // -----------------------------------------------------------------------
    // Locate the ROMS directory.
    // ROMS_PATH is the absolute path baked in at configure time by CMake.
    // argv[1] overrides it.
    // -----------------------------------------------------------------------
    const std::filesystem::path romsPath = (argc > 1) ? argv[1] : ROMS_PATH;

    {
        std::error_code ec;
        if (!std::filesystem::is_directory(romsPath, ec))
        {
            std::fprintf(stderr,
                         "[ROM] ROMS directory not found: %s\n"
                         "[ROM] Either pass the path as argv[1], or ensure ROMS/ exists at:\n"
                         "[ROM]   " ROMS_PATH "\n",
                         romsPath.string().c_str());
            return 1;
        }
    }

    // -----------------------------------------------------------------------
    // SDL initialisation
    // -----------------------------------------------------------------------
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("CPS-1 Emulator", CPS1_WIDTH * WINDOW_SCALE,
                                          CPS1_HEIGHT * WINDOW_SCALE, SDL_WINDOW_RESIZABLE);

    if (!window)
    {
        SDL_Log("SDL_CreateWindow: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);

    if (!renderer)
    {
        SDL_Log("SDL_CreateRenderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_SetRenderLogicalPresentation(renderer, CPS1_WIDTH, CPS1_HEIGHT,
                                     SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);

    SDL_Texture *framebuffer = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, CPS1_WIDTH, CPS1_HEIGHT);

    if (!framebuffer)
    {
        SDL_Log("SDL_CreateTexture: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    {
        void *pixels = nullptr;
        int pitch = 0;
        SDL_LockTexture(framebuffer, nullptr, &pixels, &pitch);
        std::memset(pixels, 0, static_cast<std::size_t>(pitch) * CPS1_HEIGHT);
        SDL_UnlockTexture(framebuffer);
    }

    // -----------------------------------------------------------------------
    // ROM scan
    // -----------------------------------------------------------------------
    LOG("[ROM] Using ROMS path: %s", romsPath.string().c_str());
    cps1::FilesystemRomSource romSource(romsPath);
    romSource.scan();

    // -----------------------------------------------------------------------
    // State machine: Bootloader → Emulating
    // -----------------------------------------------------------------------
    enum class State
    {
        Bootloader,
        Emulating
    };

    State state = State::Bootloader;

    // progRom/gfxRom own the assembled ROM bytes; Cps1System only holds
    // non-owning spans into them (see Cps1System.h), so they must outlive
    // `system` — declaring them first guarantees that via reverse-order
    // destruction at scope exit.
    std::vector<uint8_t> progRom;
    std::vector<uint8_t> gfxRom;
    std::unique_ptr<cps1::Cps1System> system;

    cps1::SdlMenuRenderer menuRenderer(renderer);
    cps1::BootloaderMenu<cps1::SdlMenuRenderer> menu(menuRenderer);
    menu.setGames(romSource.availableGames());

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
                state == State::Bootloader)
            {
                running = false;
                break;
            }

            if (state == State::Bootloader)
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
                    LOG("[EMU] Loading: %s", sel->title);

                    // Destroy any previous system before mutating the
                    // buffers it may still be pointing at.
                    system.reset();

                    if (cps1::assembleRomImages(romSource, *sel, progRom, gfxRom))
                    {
                        system = std::make_unique<cps1::Cps1System>();
                        if (system->load(progRom, gfxRom))
                        {
                            state = State::Emulating;
                            SDL_SetWindowTitle(window, sel->title);
                        }
                        else
                        {
                            SDL_Log("[EMU] Failed to load '%s' — returning to menu", sel->title);
                            system.reset();
                        }
                    }
                    else
                    {
                        SDL_Log("[EMU] Failed to assemble ROM image for '%s' — returning to menu",
                                sel->title);
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (state == State::Bootloader)
        {
            menu.render();
        }
        else
        {
            system->tick();

            // Blit the RGB565 framebuffer produced by Cps1System into the
            // SDL streaming texture and render it to the window.
            void *pixels = nullptr;
            int pitch = 0;
            SDL_LockTexture(framebuffer, nullptr, &pixels, &pitch);
            const auto src = system->framebuffer();
            std::memcpy(pixels, src.data(), src.size_bytes());
            SDL_UnlockTexture(framebuffer);

            SDL_RenderTexture(renderer, framebuffer, nullptr, nullptr);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(framebuffer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
