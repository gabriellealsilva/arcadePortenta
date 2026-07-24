#pragma once
#include "core/RomLayout.h"

// ---------------------------------------------------------------------------
// Captain Commando — captcomm
// 2 standalone word-swapped chips + 1 interleaved pair (program);
// 8 GFX chips in 2 groups of 4.
// ---------------------------------------------------------------------------
namespace cps1::games
{

inline constexpr RomSlot kCaptcommProg[] =
{
    P16S("cce_23f.8f", 0x80000, 0x000000),
    P16S("cc_22f.7f",  0x80000, 0x080000),
    P16 ("cc_24f.9e",  0x20000, 0x100000, 0),
    P16 ("cc_28f.9f",  0x20000, 0x100000, 1),
};

inline constexpr RomSlot kCaptcommGfx[] =
{
    // Group 0 — 0x000000
    G64("cc-5m.3a",  0x80000, 0x000000, 0),
    G64("cc-7m.5a",  0x80000, 0x000000, 2),
    G64("cc-1m.4a",  0x80000, 0x000000, 4),
    G64("cc-3m.6a",  0x80000, 0x000000, 6),
    // Group 1 — 0x200000
    G64("cc-6m.7a",  0x80000, 0x200000, 0),
    G64("cc-8m.9a",  0x80000, 0x200000, 2),
    G64("cc-2m.8a",  0x80000, 0x200000, 4),
    G64("cc-4m.10a", 0x80000, 0x200000, 6),
};

inline constexpr GameLayout kCaptcomm =
{
    "captcomm", "Captain Commando",
    kCaptcommProg, 0x140000,  // 2 contiguous 512KB chips + 1 interleaved 256KB pair
    kCaptcommGfx,  0x400000,
};

} // namespace cps1::games
