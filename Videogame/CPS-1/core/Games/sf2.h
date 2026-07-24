#pragma once
#include "core/RomLayout.h"

// ---------------------------------------------------------------------------
// Street Fighter II: The World Warrior — sf2
// GFX ROMs not present in ROMS/sf2/ (only a placeholder file).
// ---------------------------------------------------------------------------
namespace cps1::games
{

inline constexpr RomSlot kSf2Prog[] =
{
    P16("sf2e_28g.9e",  0x20000, 0x000000, 0),
    P16("sf2_29b.10e",  0x20000, 0x000000, 1),
    P16("sf2e_30g.11e", 0x20000, 0x040000, 0),
    P16("sf2e_31g.12e", 0x20000, 0x040000, 1),
    P16("sf2e_35g.9f",  0x20000, 0x080000, 0),
    P16("sf2_36b.10f",  0x20000, 0x080000, 1),
    P16("sf2e_37g.11f", 0x20000, 0x0C0000, 0),
    P16("sf2e_38g.12f", 0x20000, 0x0C0000, 1),
};

inline constexpr GameLayout kSf2 =
{
    "sf2", "Street Fighter II: The World Warrior",
    kSf2Prog, 0x100000,
    {},      0,           // no GFX ROMs available
};

} // namespace cps1::games
