#pragma once
#include "core/RomLayout.h"

// ---------------------------------------------------------------------------
// Mega Twins — mtwins
// 2 interleaved pairs + 1 standalone word-swapped chip (program);
// 4 GFX chips forming one complete 4-bitplane group.
// ---------------------------------------------------------------------------
namespace cps1::games
{

inline constexpr RomSlot kMtwinsProg[] =
{
    P16 ("che_30.11f", 0x20000, 0x000000, 0),
    P16 ("che_35.11h", 0x20000, 0x000000, 1),
    P16 ("che_31.12f", 0x20000, 0x040000, 0),
    P16 ("che_36.12h", 0x20000, 0x040000, 1),
    P16S("ck-32m.8h",  0x80000, 0x080000),
};

inline constexpr RomSlot kMtwinsGfx[] =
{
    G64("ck-5m.7a", 0x80000, 0x000000, 0),
    G64("ck-7m.9a", 0x80000, 0x000000, 2),
    G64("ck-1m.3a", 0x80000, 0x000000, 4),
    G64("ck-3m.5a", 0x80000, 0x000000, 6),
};

inline constexpr GameLayout kMtwins =
{
    "mtwins", "Mega Twins",
    kMtwinsProg, 0x100000,
    kMtwinsGfx,  0x200000,
};

} // namespace cps1::games
