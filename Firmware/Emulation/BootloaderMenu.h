#pragma once
#ifndef SDL_MAIN_HANDLED
#  define SDL_MAIN_HANDLED
#endif
#include <SDL3/SDL.h>
#include "Rom/RomDefs.h"
#include <optional>
#include <span>

namespace cps1
{

/// @brief GRUB-style boot menu rendered on the CPS-1 logical canvas (384 × 224 px).
///
/// @c SDL_RenderDebugText uses an 8 × 8 px bitmap font, giving a 48 × 28
/// character grid. The menu is stateless between frames.
///
/// Typical usage per frame:
/// @code
///   while (SDL_PollEvent(&ev))
///       if (auto* game = menu.handleEvent(ev)) { /* launch */ }
///   menu.render();
///   SDL_RenderPresent(renderer);
/// @endcode
class BootloaderMenu
{
public:
    /// @brief Construct with the SDL renderer that will receive draw calls.
    /// @param renderer Must remain valid for the lifetime of this object.
    explicit BootloaderMenu(SDL_Renderer* renderer) : renderer(renderer) {}

    /// @brief Replace the game list and reset the cursor to the first entry.
    /// @param games Non-owning view into the array returned by
    ///              @ref IRomSource::availableGames. The backing storage must
    ///              outlive this menu object.
    void setGames(std::span<const GameDescriptor> games)
    {
        this->games = games;
        sel = 0;
    }

    /// @brief Forward one SDL event to the menu.
    /// @return The selected @ref GameDescriptor (by value) when ENTER is pressed,
    ///         @c std::nullopt for any other event.
    std::optional<GameDescriptor> handleEvent(const SDL_Event& ev)
    {
        if (ev.type != SDL_EVENT_KEY_DOWN) return std::nullopt;

        const int n = static_cast<int>(games.size());
        switch (ev.key.key)
        {
            case SDLK_UP:
                if (n > 0) sel = (sel + n - 1) % n;
                break;
            case SDLK_DOWN:
                if (n > 0) sel = (sel + 1) % n;
                break;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                if (n > 0) return games[sel];
                break;
            default:
                break;
        }
        return std::nullopt;
    }

    /// @brief Draw one frame of the menu into the current renderer target.
    ///
    /// Canvas is 384 × 224 px. Character grid is 48 × 28 cells (8 × 8 px each).
    /// Box occupies columns 2–45, leaving a 2-cell margin on each side.
    void render()
    {
        // BOX_X = col 2 (16 px), BOX_W = 44 chars (352 px), right border at col 45.
        static constexpr float CH    = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;
        static constexpr float BOX_X = CH * 2;
        static constexpr float BOX_W = CH * 44;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDebugText(renderer, BOX_X, CH,
                            "GNU GRUB  version 0.1          CPS-1 Arcade");

        const float boxTopY     = CH * 3;
        const float entryStartY = boxTopY + CH;

        SDL_RenderDebugText(renderer, BOX_X, boxTopY,
                            "+------------------------------------------+");

        if (games.empty())
        {
            SDL_RenderDebugText(renderer, BOX_X, entryStartY,
                                "|  No ROM sets found in ROMS/ directory.  |");
            SDL_RenderDebugText(renderer, BOX_X, entryStartY + CH,
                                "+------------------------------------------+");
        }
        else
        {
            for (int i = 0; i < static_cast<int>(games.size()); ++i)
            {
                const float y     = entryStartY + CH * static_cast<float>(i);
                const char* title = games[i].title;

                if (i == sel)
                {
                    // GRUB-style inverted highlight: filled white rect, black text on top.
                    const SDL_FRect hi = { BOX_X, y, BOX_W, CH };
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    SDL_RenderFillRect(renderer, &hi);
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    SDL_RenderDebugText(renderer, BOX_X,           y, "|");
                    SDL_RenderDebugText(renderer, BOX_X + CH,      y, "*");
                    SDL_RenderDebugText(renderer, BOX_X + CH * 3,  y, title);
                    SDL_RenderDebugText(renderer, BOX_X + CH * 43, y, "|");
                }
                else
                {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    SDL_RenderDebugText(renderer, BOX_X,           y, "|");
                    SDL_RenderDebugText(renderer, BOX_X + CH * 3,  y, title);
                    SDL_RenderDebugText(renderer, BOX_X + CH * 43, y, "|");
                }
            }

            const float boxBottomY = entryStartY + CH * static_cast<float>(games.size());
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDebugText(renderer, BOX_X, boxBottomY,
                                "+------------------------------------------+");

            const float footY = boxBottomY + CH * 2;
            SDL_RenderDebugText(renderer, BOX_X, footY,
                                "Use the UP and DOWN arrow keys to select.");
            SDL_RenderDebugText(renderer, BOX_X, footY + CH,
                                "Press ENTER to boot.  ESC to quit.");
        }
    }

private:
    SDL_Renderer*                   renderer;
    std::span<const GameDescriptor> games;
    int                             sel = 0;
};

} // namespace cps1
