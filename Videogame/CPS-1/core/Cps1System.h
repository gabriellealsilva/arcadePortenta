#pragma once
#include <cstdint>
#include <span>

namespace cps1
{

/// @brief Top-level CPS-1 hardware emulation state.
///
/// Owns the RAM regions (work RAM, GFX RAM, palette RAM) and the rendered
/// framebuffer — but deliberately does NOT own program/GFX ROM storage.
/// @ref load takes non-owning spans over an already-assembled ROM image:
///  - On the host emulator, those bytes live in caller-owned vectors built
///    by @c assembleRomImages (Firmware/Emulation/RomAssembler.h) from
///    individual chip-dump files.
///  - On hardware, they will instead be spans directly over the QSPI
///    XIP-mapped address range — no copy, no allocation — assembled once
///    ahead of time by flash mode, not on every boot.
/// This keeps the emulation core itself platform-agnostic and allocation-free;
/// only the ROM *assembly* step differs per backend.
///
/// The caller must keep the backing storage for both spans alive for as
/// long as this object is used (until the next @ref load or destruction).
///
/// Drives Musashi via the global callback table defined in @c Cps1System.cpp.
/// Only one instance may be alive at a time (Musashi uses a global dispatch
/// pointer that is set in @ref load and cleared in the destructor).
class Cps1System
{
public:
    Cps1System();
    ~Cps1System();

    Cps1System(const Cps1System&)            = delete;
    Cps1System& operator=(const Cps1System&) = delete;

    /// @brief Wire up an already-assembled ROM image and reset the 68000.
    /// @param progRom Program ROM bytes; see class docs for lifetime rules.
    /// @param gfxRom  GFX ROM bytes; may be empty if the game has none dumped.
    /// @return @c false if @p progRom is empty (nothing to execute).
    bool load(std::span<const uint8_t> progRom, std::span<const uint8_t> gfxRom);

    /// @brief Execute one video frame (~167 k cycles), fire VBlank IRQ, render.
    void tick();

    /// @brief RGB565 framebuffer, 384 × 224 pixels in row-major order.
    std::span<const uint16_t> framebuffer() const { return fb; }

    // -----------------------------------------------------------------------
    // Memory dispatch — public so the global Musashi C callbacks can reach
    // them.  Not part of the external API; do not call directly.
    // -----------------------------------------------------------------------
    uint32_t read8 (uint32_t addr) const;
    uint32_t read16(uint32_t addr) const;
    uint32_t read32(uint32_t addr) const;
    void     write8 (uint32_t addr, uint32_t val);
    void     write16(uint32_t addr, uint32_t val);
    void     write32(uint32_t addr, uint32_t val);

private:
    void renderFrame();

    static constexpr int WIDTH  = 384;
    static constexpr int HEIGHT = 224;

    std::span<const uint8_t> progRom;                ///< Non-owning; see class docs
    std::span<const uint8_t> gfxRom;                 ///< Non-owning; see class docs
    uint8_t  workRam   [64  * 1024] = {};            ///< CPU work RAM   (0xFF0000–0xFFFFFF)
    uint8_t  gfxRam    [512 * 1024] = {};            ///< GFX / object RAM (0x800000–0x87FFFF)
    uint8_t  paletteRam[128 * 1024] = {};            ///< Palette RAM    (0x900000–0x91FFFF)
    uint16_t fb        [WIDTH * HEIGHT] = {};        ///< RGB565 output framebuffer

    /// CPS-A write-only registers, base 0xC00000, index = (addr - 0xC00000) / 2.
    /// [0]  0xC00000  Scroll1 X      [1]  0xC00002  Scroll1 Y
    /// [2]  0xC00004  Scroll2 X      [3]  0xC00006  Scroll2 Y
    /// [4]  0xC00008  Scroll3 X      [5]  0xC0000A  Scroll3 Y
    /// [8]  0xC00010  Scroll1 base   [9]  0xC00012  Scroll2 base
    /// [10] 0xC00014  Scroll3 base   [11] 0xC00016  OBJ base
    uint16_t cpsARegs[32] = {};
};

} // namespace cps1
