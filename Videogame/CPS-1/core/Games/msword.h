#pragma once
#include "core/RomLayout.h"

// ---------------------------------------------------------------------------
// Magic Sword — msword
// 2 interleaved pairs + 1 standalone word-swapped chip (program);
// 4 GFX chips forming one complete 4-bitplane group.
// ---------------------------------------------------------------------------
namespace cps1::games
{

inline constexpr RomSlot kMswordProg[] =
{
    P16 ("mse_30.11f", 0x20000, 0x000000, 0),
    P16 ("mse_35.11h", 0x20000, 0x000000, 1),
    P16 ("mse_31.12f", 0x20000, 0x040000, 0),
    P16 ("mse_36.12h", 0x20000, 0x040000, 1),
    P16S("ms-32m.8h",  0x80000, 0x080000),
};

inline constexpr RomSlot kMswordGfx[] =
{
    G64("ms-5m.7a", 0x80000, 0x000000, 0),
    G64("ms-7m.9a", 0x80000, 0x000000, 2),
    G64("ms-1m.3a", 0x80000, 0x000000, 4),
    G64("ms-3m.5a", 0x80000, 0x000000, 6),
};

inline constexpr GameLayout kMsword =
{
    "msword", "Magic Sword: Heroic Fantasy",
    kMswordProg, 0x100000,
    kMswordGfx,  0x200000,
};

} // namespace cps1::games
