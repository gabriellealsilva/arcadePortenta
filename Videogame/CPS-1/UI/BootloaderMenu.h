#pragma once
#include "Rom/RomDefs.h"
#include <concepts>
#include <cstdint>
#include <optional>
#include <span>

namespace cps1
{

/// @brief Simple RGBA color, platform-agnostic (no SDL/HAL type leaks into this header).
struct Color
{
    std::uint8_t r, g, b, a = 255;
};

/// @brief Directional/confirm input the menu reacts to, independent of the
/// underlying input source (SDL keyboard on host, JAMMA buttons on hardware).
enum class MenuInput
{
    Up,
    Down,
    Confirm
};

/// @brief Drawing primitives a @ref BootloaderMenu backend must provide.
///
/// The host emulator implements this over SDL_RenderDebugText/SDL_RenderFillRect;
/// hardware will implement it over an LTDC glyph blitter. Checked via
/// @c concept rather than virtual dispatch, matching @ref IRomSource.
template<typename T>
concept MenuRenderer = requires(T& r, float x, float y, float w, float h, const char* text, Color c)
{
    { r.clear(c) };
    { r.setColor(c) };
    { r.fillRect(x, y, w, h) };
    { r.drawText(x, y, text) };
};

/// @brief GRUB-style boot menu rendered on the CPS-1 logical canvas (384 x 224 px).
///
/// Assumes an 8 x 8 px bitmap font, giving a 48 x 28 character grid. The menu
/// is stateless between frames except for the current selection index.
///
/// Platform-agnostic: all drawing goes through a @ref MenuRenderer-conforming
/// backend, and all input arrives as @ref MenuInput, so this header has no
/// SDL or STM32 HAL dependency and compiles for both the host emulator and
/// the firmware.
///
/// Typical usage per frame:
/// @code
///   if (auto* game = menu.handleInput(input)) { /* launch */ }
///   menu.render();
/// @endcode
///
/// @tparam Renderer Concrete backend satisfying @ref MenuRenderer.
template<MenuRenderer Renderer>
class BootloaderMenu
{
public:
    /// @brief Construct with the renderer that will receive draw calls.
    /// @param renderer Must remain valid for the lifetime of this object.
    explicit BootloaderMenu(Renderer& renderer) : renderer(renderer) {}

    /// @brief Replace the game list and reset the cursor to the first entry.
    /// @param games Non-owning view into the array returned by
    ///              @ref IRomSource::availableGames. The backing storage must
    ///              outlive this menu object.
    void setGames(std::span<const GameDescriptor> games)
    {
        this->games = games;
        sel = 0;
    }

    /// @brief Feed one input event to the menu.
    /// @return The selected @ref GameDescriptor (by value) on @ref MenuInput::Confirm,
    ///         @c std::nullopt otherwise.
    std::optional<GameDescriptor> handleInput(MenuInput input)
    {
        const int n = static_cast<int>(games.size());
        switch (input)
        {
            case MenuInput::Up:
                if (n > 0) sel = (sel + n - 1) % n;
                break;
            case MenuInput::Down:
                if (n > 0) sel = (sel + 1) % n;
                break;
            case MenuInput::Confirm:
                if (n > 0) return games[sel];
                break;
        }
        return std::nullopt;
    }

    /// @brief Draw one frame of the menu via the backend renderer.
    ///
    /// Canvas is 384 x 224 px. Character grid is 48 x 28 cells (8 x 8 px each).
    /// Box occupies columns 2-45, leaving a 2-cell margin on each side.
    void render()
    {
        // BOX_X = col 2 (16 px), BOX_W = 44 chars (352 px), right border at col 45.
        static constexpr float CH    = 8.0f;
        static constexpr float BOX_X = CH * 2;
        static constexpr float BOX_W = CH * 44;

        renderer.clear(Color{0, 0, 0});

        renderer.setColor(Color{255, 255, 255});
        renderer.drawText(BOX_X, CH,
                          "GNU GRUB  version 0.1          CPS-1 Arcade");

        const float boxTopY     = CH * 3;
        const float entryStartY = boxTopY + CH;

        renderer.drawText(BOX_X, boxTopY,
                          "+------------------------------------------+");

        if (games.empty())
        {
            renderer.drawText(BOX_X, entryStartY,
                              "|  No ROM sets found in ROMS/ directory.  |");
            renderer.drawText(BOX_X, entryStartY + CH,
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
                    renderer.setColor(Color{255, 255, 255});
                    renderer.fillRect(BOX_X, y, BOX_W, CH);
                    renderer.setColor(Color{0, 0, 0});
                    renderer.drawText(BOX_X,           y, "|");
                    renderer.drawText(BOX_X + CH,      y, "*");
                    renderer.drawText(BOX_X + CH * 3,  y, title);
                    renderer.drawText(BOX_X + CH * 43, y, "|");
                }
                else
                {
                    renderer.setColor(Color{255, 255, 255});
                    renderer.drawText(BOX_X,           y, "|");
                    renderer.drawText(BOX_X + CH * 3,  y, title);
                    renderer.drawText(BOX_X + CH * 43, y, "|");
                }
            }

            const float boxBottomY = entryStartY + CH * static_cast<float>(games.size());
            renderer.setColor(Color{255, 255, 255});
            renderer.drawText(BOX_X, boxBottomY,
                              "+------------------------------------------+");

            const float footY = boxBottomY + CH * 2;
            renderer.drawText(BOX_X, footY,
                              "Use the UP and DOWN arrow keys to select.");
            renderer.drawText(BOX_X, footY + CH,
                              "Press ENTER to boot.  ESC to quit.");
        }
    }

private:
    Renderer&                       renderer;
    std::span<const GameDescriptor> games;
    int                             sel = 0;
};

} // namespace cps1
