#pragma once
#include "core/RomLayout.h"

// ---------------------------------------------------------------------------
// Street Fighter II: The World Warrior — sf2 (MAME cps1.cpp, ROM_START(sf2))
// 8 program chips (4 even/odd pairs), 12 GFX chips in 3 groups of 4.
// ---------------------------------------------------------------------------
namespace cps1::games
{

inline constexpr RomSlot kSf2Prog[] =
{
    P16("sf2e_30g.11e", 0x20000, 0x000000, 0),
    P16("sf2e_37g.11f", 0x20000, 0x000000, 1),
    P16("sf2e_31g.12e", 0x20000, 0x040000, 0),
    P16("sf2e_38g.12f", 0x20000, 0x040000, 1),
    P16("sf2e_28g.9e",  0x20000, 0x080000, 0),
    P16("sf2e_35g.9f",  0x20000, 0x080000, 1),
    P16("sf2_29b.10e",  0x20000, 0x0C0000, 0),
    P16("sf2_36b.10f",  0x20000, 0x0C0000, 1),
};

inline constexpr RomSlot kSf2Gfx[] =
{
    G64("sf2-5m.4a",  0x80000, 0x000000, 0),
    G64("sf2-7m.6a",  0x80000, 0x000000, 2),
    G64("sf2-1m.3a",  0x80000, 0x000000, 4),
    G64("sf2-3m.5a",  0x80000, 0x000000, 6),
    G64("sf2-6m.4c",  0x80000, 0x200000, 0),
    G64("sf2-8m.6c",  0x80000, 0x200000, 2),
    G64("sf2-2m.3c",  0x80000, 0x200000, 4),
    G64("sf2-4m.5c",  0x80000, 0x200000, 6),
    G64("sf2-13m.4d", 0x80000, 0x400000, 0),
    G64("sf2-15m.6d", 0x80000, 0x400000, 2),
    G64("sf2-9m.3d",  0x80000, 0x400000, 4),
    G64("sf2-11m.5d", 0x80000, 0x400000, 6),
};

inline constexpr GameLayout kSf2 =
{
    "sf2", "Street Fighter II: The World Warrior",
    kSf2Prog, 0x100000,
    kSf2Gfx,  0x600000,
};

} // namespace cps1::games
