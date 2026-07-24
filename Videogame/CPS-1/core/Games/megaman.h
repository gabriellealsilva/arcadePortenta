#pragma once
#include "core/RomLayout.h"

// ---------------------------------------------------------------------------
// Mega Man: The Power Battle — megaman
// 16 GFX chips in 4 groups of 4, each chip 512 KB.
// ---------------------------------------------------------------------------
namespace cps1::games
{

inline constexpr RomSlot kMegamanProg[] =
{
    P16("rcmu_21a.6f", 0x80000, 0x000000, 0),
    P16("rcmu_22b.7f", 0x80000, 0x000000, 1),
    P16("rcmu_23b.8f", 0x80000, 0x100000, 1),
};

inline constexpr RomSlot kMegamanGfx[] =
{
    // Group 0 — 0x000000
    G64("rcm_01.3a",  0x80000, 0x000000, 0),
    G64("rcm_02.4a",  0x80000, 0x000000, 2),
    G64("rcm_03.5a",  0x80000, 0x000000, 4),
    G64("rcm_04.6a",  0x80000, 0x000000, 6),
    // Group 1 — 0x200000
    G64("rcm_05.7a",  0x80000, 0x200000, 0),
    G64("rcm_06.8a",  0x80000, 0x200000, 2),
    G64("rcm_07.9a",  0x80000, 0x200000, 4),
    G64("rcm_08.10a", 0x80000, 0x200000, 6),
    // Group 2 — 0x400000
    G64("rcm_10.3c",  0x80000, 0x400000, 0),
    G64("rcm_11.4c",  0x80000, 0x400000, 2),
    G64("rcm_12.5c",  0x80000, 0x400000, 4),
    G64("rcm_13.6c",  0x80000, 0x400000, 6),
    // Group 3 — 0x600000
    G64("rcm_14.7c",  0x80000, 0x600000, 0),
    G64("rcm_15.8c",  0x80000, 0x600000, 2),
    G64("rcm_16.9c",  0x80000, 0x600000, 4),
    G64("rcm_17.10c", 0x80000, 0x600000, 6),
};

inline constexpr GameLayout kMegaman =
{
    "megaman", "Mega Man: The Power Battle",
    kMegamanProg, 0x200000,  // same layout as sf2ce; third chip reaches 0x1FFFFF
    kMegamanGfx,  0x800000,
};

} // namespace cps1::games
