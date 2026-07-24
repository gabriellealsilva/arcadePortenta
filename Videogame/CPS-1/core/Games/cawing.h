#pragma once
#include "core/RomLayout.h"

// ---------------------------------------------------------------------------
// Carrier Air Wing — cawing
// 2 interleaved pairs + 1 standalone word-swapped chip (program);
// 4 GFX chips forming one complete 4-bitplane group.
// ---------------------------------------------------------------------------
namespace cps1::games
{

inline constexpr RomSlot kCawingProg[] =
{
    P16 ("cae_30a.11f", 0x20000, 0x000000, 0),
    P16 ("cae_35a.11h", 0x20000, 0x000000, 1),
    P16 ("cae_31a.12f", 0x20000, 0x040000, 0),
    P16 ("cae_36a.12h", 0x20000, 0x040000, 1),
    P16S("ca-32m.8h",   0x80000, 0x080000),
};

inline constexpr RomSlot kCawingGfx[] =
{
    G64("ca-5m.7a", 0x80000, 0x000000, 0),
    G64("ca-7m.9a", 0x80000, 0x000000, 2),
    G64("ca-1m.3a", 0x80000, 0x000000, 4),
    G64("ca-3m.5a", 0x80000, 0x000000, 6),
};

inline constexpr GameLayout kCawing =
{
    "cawing", "Carrier Air Wing",
    kCawingProg, 0x100000,
    kCawingGfx,  0x200000,
};

} // namespace cps1::games
