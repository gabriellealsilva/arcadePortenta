#pragma once
#include "core/RomLayout.h"

// ---------------------------------------------------------------------------
// King of Dragons — kod
// 4 interleaved pairs (program, same shape as 3wonders);
// 8 GFX chips in 2 groups of 4.
// ---------------------------------------------------------------------------
namespace cps1::games
{

inline constexpr RomSlot kKodProg[] =
{
    P16("kde_30a.11e", 0x20000, 0x000000, 0),
    P16("kde_37a.11f", 0x20000, 0x000000, 1),
    P16("kde_31a.12e", 0x20000, 0x040000, 0),
    P16("kde_38a.12f", 0x20000, 0x040000, 1),
    P16("kd_28.9e",    0x20000, 0x080000, 0),
    P16("kd_35.9f",    0x20000, 0x080000, 1),
    P16("kd_29.10e",   0x20000, 0x0C0000, 0),
    P16("kd_36a.10f",  0x20000, 0x0C0000, 1),
};

inline constexpr RomSlot kKodGfx[] =
{
    // Group 0 — 0x000000
    G64("kd-5m.4a", 0x80000, 0x000000, 0),
    G64("kd-7m.6a", 0x80000, 0x000000, 2),
    G64("kd-1m.3a", 0x80000, 0x000000, 4),
    G64("kd-3m.5a", 0x80000, 0x000000, 6),
    // Group 1 — 0x200000
    G64("kd-6m.4c", 0x80000, 0x200000, 0),
    G64("kd-8m.6c", 0x80000, 0x200000, 2),
    G64("kd-2m.3c", 0x80000, 0x200000, 4),
    G64("kd-4m.5c", 0x80000, 0x200000, 6),
};

inline constexpr GameLayout kKod =
{
    "kod", "King of Dragons",
    kKodProg, 0x100000,
    kKodGfx,  0x400000,
};

} // namespace cps1::games
