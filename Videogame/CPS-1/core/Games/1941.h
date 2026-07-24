#pragma once
#include "core/RomLayout.h"

// ---------------------------------------------------------------------------
// 1941: Counter Attack — 1941
// 4 GFX chips forming one complete 4-bitplane group.
// ---------------------------------------------------------------------------
namespace cps1::games
{

inline constexpr RomSlot k1941Prog[] =
{
    P16("41em_30.11f", 0x20000, 0x000000, 0),
    P16("41em_31.12f", 0x20000, 0x000000, 1),
    P16("41em_35.11h", 0x20000, 0x040000, 0),
    P16("41em_36.12h", 0x20000, 0x040000, 1),
};

inline constexpr RomSlot k1941Gfx[] =
{
    G64("41-1m.3a",  0x80000, 0x000000, 0),
    G64("41-3m.5a",  0x80000, 0x000000, 2),
    G64("41-5m.7a",  0x80000, 0x000000, 4),
    G64("41-7m.9a",  0x80000, 0x000000, 6),
};

inline constexpr GameLayout k1941 =
{
    "1941", "1941: Counter Attack",
    k1941Prog, 0x080000,
    k1941Gfx,  0x200000,
};

} // namespace cps1::games
