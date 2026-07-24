#pragma once
#include "core/RomLayout.h"

// ---------------------------------------------------------------------------
// Mercs — mercs
// 2 interleaved pairs + 1 standalone word-swapped chip (program);
// 4 GFX word chips (one group) + 8 GFX byte chips (one group) after them.
// ---------------------------------------------------------------------------
namespace cps1::games
{

inline constexpr RomSlot kMercsProg[] =
{
    P16 ("so2_30e.11f", 0x20000, 0x000000, 0),
    P16 ("so2_35e.11h", 0x20000, 0x000000, 1),
    P16 ("so2_31e.12f", 0x20000, 0x040000, 0),
    P16 ("so2_36e.12h", 0x20000, 0x040000, 1),
    P16S("so2-32m.8h",  0x80000, 0x080000),
};

inline constexpr RomSlot kMercsGfx[] =
{
    G64 ("so2-6m.8a",  0x80000, 0x000000, 0),
    G64 ("so2-8m.10a", 0x80000, 0x000000, 2),
    G64 ("so2-2m.4a",  0x80000, 0x000000, 4),
    G64 ("so2-4m.6a",  0x80000, 0x000000, 6),
    G64B("so2_24.7d",  0x20000, 0x200000, 0),
    G64B("so2_14.7c",  0x20000, 0x200000, 1),
    G64B("so2_26.9d",  0x20000, 0x200000, 2),
    G64B("so2_16.9c",  0x20000, 0x200000, 3),
    G64B("so2_20.3d",  0x20000, 0x200000, 4),
    G64B("so2_10.3c",  0x20000, 0x200000, 5),
    G64B("so2_22.5d",  0x20000, 0x200000, 6),
    G64B("so2_12.5c",  0x20000, 0x200000, 7),
};

inline constexpr GameLayout kMercs =
{
    "mercs", "Mercs",
    kMercsProg, 0x100000,
    kMercsGfx,  0x300000,
};

} // namespace cps1::games
