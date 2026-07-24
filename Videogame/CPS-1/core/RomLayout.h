#pragma once
#include "Rom/RomDefs.h"
#include <span>

namespace cps1
{

/// @brief Describes how one physical ROM chip maps into a hardware bus.
///
/// Covers three MAME load macros:
///  - @c ROM_LOAD16_BYTE      : @c stride=2, @c groupBytes=1
///  - @c ROM_LOAD64_WORD      : @c stride=8, @c groupBytes=2
///  - @c ROM_LOAD64_BYTE      : @c stride=8, @c groupBytes=1
///  - @c ROM_LOAD16_WORD_SWAP : @c stride=2, @c groupBytes=2, @c swap=true
///    (a standalone, contiguous 16-bit-wide chip — not interleaved with a
///    partner — whose two bytes per word must be swapped on the way in)
///
/// Loading formula for byte @c i of the source file:
/// @code
///   group       = i / groupBytes
///   byteInGroup = i % groupBytes
///   effByte     = swap ? (groupBytes - 1 - byteInGroup) : byteInGroup
///   dest[destOffset + group * stride + destByte + effByte] = raw[i]
/// @endcode
struct RomSlot
{
    const char* filename;
    uint32_t    size;
    RomRegion   region;
    uint32_t    destOffset; ///< First destination byte in the region buffer
    uint8_t     stride;     ///< Bytes per stride group in destination (2 or 8)
    uint8_t     groupBytes; ///< Bytes per stride group in the source file (1 or 2)
    uint8_t     destByte;   ///< Byte lane offset within the stride group
    bool        swap = false; ///< Reverse byte order within each group (ROM_LOAD16_WORD_SWAP)
};

// ---------------------------------------------------------------------------
// Shorthand helpers for building RomSlot tables from MAME ROM_LOAD lines.
// ---------------------------------------------------------------------------
#define P16(f, sz, off, b)   RomSlot{ f, sz, RomRegion::Program68k, off, 2, 1, b, false }
#define P16S(f, sz, off)     RomSlot{ f, sz, RomRegion::Program68k, off, 2, 2, 0, true }
#define G64(f, sz, off, b)   RomSlot{ f, sz, RomRegion::Gfx,        off, 8, 2, b, false }
#define G64B(f, sz, off, b)  RomSlot{ f, sz, RomRegion::Gfx,        off, 8, 1, b, false }

/// @brief Complete ROM loading description for one CPS-1 game set.
struct GameLayout
{
    const char*              id;
    const char*              title;
    std::span<const RomSlot> progSlots;    ///< 68000 program ROM (ROM_LOAD16_BYTE)
    uint32_t                 progRomSize;  ///< Program ROM buffer size (filled 0xFF)
    std::span<const RomSlot> gfxSlots;    ///< Graphics ROMs (ROM_LOAD64_WORD)
    uint32_t                 gfxRomSize;  ///< GFX ROM buffer size (filled 0x00)
};

/// @brief Find the loading layout for a game by its MAME set ID.
/// @return Pointer to the layout, or @c nullptr if the game is unrecognised.
const GameLayout* findGameLayout(const char* id);

} // namespace cps1
