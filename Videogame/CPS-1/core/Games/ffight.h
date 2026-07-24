#pragma once
#include "core/RomLayout.h"

// ---------------------------------------------------------------------------
// Final Fight — ffight
// 2 interleaved pairs + 1 standalone word-swapped chip (program);
// 4 GFX chips forming one complete 4-bitplane group.
// ---------------------------------------------------------------------------
namespace cps1::games
{

inline constexpr RomSlot kFfightProg[] =
{
    P16 ("ff_36.11f",  0x20000, 0x000000, 0),
    P16 ("ff_42.11h",  0x20000, 0x000000, 1),
    P16 ("ff_37.12f",  0x20000, 0x040000, 0),
    P16 ("ffe_43.12h", 0x20000, 0x040000, 1),
    P16S("ff-32m.8h",  0x80000, 0x080000),
};

inline constexpr RomSlot kFfightGfx[] =
{
    G64("ff-5m.7a", 0x80000, 0x000000, 0),
    G64("ff-7m.9a", 0x80000, 0x000000, 2),
    G64("ff-1m.3a", 0x80000, 0x000000, 4),
    G64("ff-3m.5a", 0x80000, 0x000000, 6),
};

inline constexpr GameLayout kFfight =
{
    "ffight", "Final Fight",
    kFfightProg, 0x100000,
    kFfightGfx,  0x200000,
};

} // namespace cps1::games
