#pragma once
#include <cstdint>
#include <span>

#include "core/CpsBConfig.h"

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
    /// @param cpsb    CPS-B board configuration for this title (see CpsBConfig.h);
    ///                must outlive this object.
    /// @return @c false if @p progRom is empty (nothing to execute).
    bool load(std::span<const uint8_t> progRom, std::span<const uint8_t> gfxRom,
              const CpsBConfig& cpsb);

    /// @brief Execute one video frame (~167 k cycles), fire VBlank IRQ, render.
    void tick();

    /// @brief RGB565 framebuffer, 384 × 224 pixels in row-major order.
    std::span<const uint16_t> framebuffer() const { return fb; }

    /// @brief Current 68000 program counter (diagnostics only).
    uint32_t programCounter() const;

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
    const CpsBConfig*        cpsb = nullptr;         ///< Non-owning; per-title CPS-B setup
    uint8_t  workRam [64  * 1024] = {};              ///< CPU work RAM (0xFF0000–0xFFFFFF)
    uint8_t  gfxRam  [192 * 1024] = {};              ///< GFX RAM (0x900000–0x92FFFF): scroll, OBJ, palette source
    uint16_t palette [0xC00] = {};                   ///< RGB565 palette, rebuilt from gfxRam (6 pages × 512)
    uint16_t fb      [WIDTH * HEIGHT] = {};          ///< RGB565 output framebuffer

    /// CPS-A write-only registers at 0x800100, index = word offset.
    /// Names follow MAME's CPS1_* constants (see CpsAReg below).
    uint16_t cpsARegs[32] = {};
    /// CPS-B registers at 0x800140 (read/write), index = word offset.
    uint16_t cpsBRegs[32] = {};

    /// CPS-A register word indices.
    enum CpsAReg : unsigned
    {
        ObjBase = 0x00 / 2, Scroll1Base = 0x02 / 2, Scroll2Base = 0x04 / 2,
        Scroll3Base = 0x06 / 2, OtherBase = 0x08 / 2, PaletteBase = 0x0a / 2,
        Scroll1X = 0x0c / 2, Scroll1Y = 0x0e / 2, Scroll2X = 0x10 / 2,
        Scroll2Y = 0x12 / 2, Scroll3X = 0x14 / 2, Scroll3Y = 0x16 / 2,
        VideoControl = 0x22 / 2,
    };

    uint32_t readIo16(uint32_t addr) const;   ///< 16-bit read of 0x800000–0x8001FF
    void     writeIo16(uint32_t addr, uint32_t val);
    void     buildPalette();
    uint16_t gfxRamWord(uint32_t byteOff) const;
};

} // namespace cps1
