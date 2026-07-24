#pragma once
#include "core/RomLayout.h"

// ---------------------------------------------------------------------------
// Knights of the Round — knights
// 2 standalone word-swapped chips, no interleaved pair (program);
// 8 GFX chips in 2 groups of 4.
// ---------------------------------------------------------------------------
namespace cps1::games
{

inline constexpr RomSlot kKnightsProg[] =
{
    P16S("kr_23e.8f", 0x80000, 0x000000),
    P16S("kr_22.7f",  0x80000, 0x080000),
};

inline constexpr RomSlot kKnightsGfx[] =
{
    // Group 0 — 0x000000
    G64("kr-5m.3a",  0x80000, 0x000000, 0),
    G64("kr-7m.5a",  0x80000, 0x000000, 2),
    G64("kr-1m.4a",  0x80000, 0x000000, 4),
    G64("kr-3m.6a",  0x80000, 0x000000, 6),
    // Group 1 — 0x200000
    G64("kr-6m.7a",  0x80000, 0x200000, 0),
    G64("kr-8m.9a",  0x80000, 0x200000, 2),
    G64("kr-2m.8a",  0x80000, 0x200000, 4),
    G64("kr-4m.10a", 0x80000, 0x200000, 6),
};

inline constexpr GameLayout kKnights =
{
    "knights", "Knights of the Round",
    kKnightsProg, 0x100000,
    kKnightsGfx,  0x400000,
};

} // namespace cps1::games
