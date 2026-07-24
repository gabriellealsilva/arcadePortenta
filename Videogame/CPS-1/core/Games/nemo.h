#pragma once
#include "core/RomLayout.h"

// ---------------------------------------------------------------------------
// Nemo — nemo
// 2 interleaved pairs + 1 standalone word-swapped chip (program);
// 4 GFX chips forming one complete 4-bitplane group.
// ---------------------------------------------------------------------------
namespace cps1::games
{

inline constexpr RomSlot kNemoProg[] =
{
    P16 ("nme_30a.11f", 0x20000, 0x000000, 0),
    P16 ("nme_35a.11h", 0x20000, 0x000000, 1),
    P16 ("nme_31a.12f", 0x20000, 0x040000, 0),
    P16 ("nme_36a.12h", 0x20000, 0x040000, 1),
    P16S("nm-32m.8h",   0x80000, 0x080000),
};

inline constexpr RomSlot kNemoGfx[] =
{
    G64("nm-5m.7a", 0x80000, 0x000000, 0),
    G64("nm-7m.9a", 0x80000, 0x000000, 2),
    G64("nm-1m.3a", 0x80000, 0x000000, 4),
    G64("nm-3m.5a", 0x80000, 0x000000, 6),
};

inline constexpr GameLayout kNemo =
{
    "nemo", "Nemo",
    kNemoProg, 0x100000,
    kNemoGfx,  0x200000,
};

} // namespace cps1::games
