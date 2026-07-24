#pragma once
#include "core/RomLayout.h"

// ---------------------------------------------------------------------------
// Three Wonders — 3wonders
// 4 interleaved pairs (program, same shape as sf2/1941);
// 8 GFX chips in 2 groups of 4.
// ---------------------------------------------------------------------------
namespace cps1::games
{

inline constexpr RomSlot k3wondersProg[] =
{
    P16("rte_30a.11f", 0x20000, 0x000000, 0),
    P16("rte_35a.11h", 0x20000, 0x000000, 1),
    P16("rte_31a.12f", 0x20000, 0x040000, 0),
    P16("rte_36a.12h", 0x20000, 0x040000, 1),
    P16("rt_28a.9f",   0x20000, 0x080000, 0),
    P16("rt_33a.9h",   0x20000, 0x080000, 1),
    P16("rte_29a.10f", 0x20000, 0x0C0000, 0),
    P16("rte_34a.10h", 0x20000, 0x0C0000, 1),
};

inline constexpr RomSlot k3wondersGfx[] =
{
    // Group 0 — 0x000000
    G64("rt-5m.7a",  0x80000, 0x000000, 0),
    G64("rt-7m.9a",  0x80000, 0x000000, 2),
    G64("rt-1m.3a",  0x80000, 0x000000, 4),
    G64("rt-3m.5a",  0x80000, 0x000000, 6),
    // Group 1 — 0x200000
    G64("rt-6m.8a",  0x80000, 0x200000, 0),
    G64("rt-8m.10a", 0x80000, 0x200000, 2),
    G64("rt-2m.4a",  0x80000, 0x200000, 4),
    G64("rt-4m.6a",  0x80000, 0x200000, 6),
};

inline constexpr GameLayout k3wonders =
{
    "3wonders", "Three Wonders",
    k3wondersProg, 0x100000,
    k3wondersGfx,  0x400000,
};

} // namespace cps1::games
