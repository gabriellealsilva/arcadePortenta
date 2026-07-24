#pragma once
#include "core/RomLayout.h"

// ---------------------------------------------------------------------------
// Quiz & Dragons: Capcom Quiz Game — qad
// 2 interleaved pairs (program);
// 8 byte-mode GFX chips forming one complete 4-bitplane group.
// ---------------------------------------------------------------------------
namespace cps1::games
{

inline constexpr RomSlot kQadProg[] =
{
    P16("qdu_36a.12f", 0x20000, 0x000000, 0),
    P16("qdu_42a.12h", 0x20000, 0x000000, 1),
    P16("qdu_37a.13f", 0x20000, 0x040000, 0),
    P16("qdu_43a.13h", 0x20000, 0x040000, 1),
};

inline constexpr RomSlot kQadGfx[] =
{
    G64B("qd_09.4b", 0x20000, 0x000000, 0),
    G64B("qd_01.4a", 0x20000, 0x000000, 1),
    G64B("qd_13.9b", 0x20000, 0x000000, 2),
    G64B("qd_05.9a", 0x20000, 0x000000, 3),
    G64B("qd_24.5e", 0x20000, 0x000000, 4),
    G64B("qd_17.5c", 0x20000, 0x000000, 5),
    G64B("qd_38.8h", 0x20000, 0x000000, 6),
    G64B("qd_32.8f", 0x20000, 0x000000, 7),
};

inline constexpr GameLayout kQad =
{
    "qad", "Quiz & Dragons: Capcom Quiz Game",
    kQadProg, 0x080000,
    kQadGfx,  0x200000,
};

} // namespace cps1::games
