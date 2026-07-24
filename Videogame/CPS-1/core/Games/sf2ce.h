#pragma once
#include "core/RomLayout.h"

// ---------------------------------------------------------------------------
// Street Fighter II': Champion Edition — sf2ce
// 12 GFX chips in 3 groups of 4, each chip 512 KB.
// ---------------------------------------------------------------------------
namespace cps1::games
{

inline constexpr RomSlot kSf2CeProg[] =
{
    P16("s92_21a.6f",  0x80000, 0x000000, 0),
    P16("s92_22b.7f",  0x80000, 0x000000, 1),
    P16("s92e_23b.8f", 0x80000, 0x100000, 1), // odd bytes only; even lane = 0xFF
};

inline constexpr RomSlot kSf2CeGfx[] =
{
    // Group 0 — 0x000000
    G64("s92-1m.3a",   0x80000, 0x000000, 0),
    G64("s92-2m.4a",   0x80000, 0x000000, 2),
    G64("s92-3m.5a",   0x80000, 0x000000, 4),
    G64("s92-4m.6a",   0x80000, 0x000000, 6),
    // Group 1 — 0x200000
    G64("s92-5m.7a",   0x80000, 0x200000, 0),
    G64("s92-6m.8a",   0x80000, 0x200000, 2),
    G64("s92-7m.9a",   0x80000, 0x200000, 4),
    G64("s92-8m.10a",  0x80000, 0x200000, 6),
    // Group 2 — 0x400000
    G64("s92-10m.3c",  0x80000, 0x400000, 0),
    G64("s92-11m.4c",  0x80000, 0x400000, 2),
    G64("s92-12m.5c",  0x80000, 0x400000, 4),
    G64("s92-13m.6c",  0x80000, 0x400000, 6),
};

inline constexpr GameLayout kSf2Ce =
{
    "sf2ce", "Street Fighter II': Champion Edition",
    kSf2CeProg, 0x200000,  // third chip writes up to 0x1FFFFF; must allocate 2 MB
    kSf2CeGfx,  0x600000,
};

} // namespace cps1::games
