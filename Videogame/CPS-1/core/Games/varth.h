#pragma once
#include "core/RomLayout.h"

// ---------------------------------------------------------------------------
// Varth: Operation Thunderstorm — varth
// 4 interleaved pairs (program, same shape as 3wonders/kod);
// 4 GFX chips forming one complete 4-bitplane group.
// ---------------------------------------------------------------------------
namespace cps1::games
{

inline constexpr RomSlot kVarthProg[] =
{
    P16("vae_30b.11f", 0x20000, 0x000000, 0),
    P16("vae_35b.11h", 0x20000, 0x000000, 1),
    P16("vae_31b.12f", 0x20000, 0x040000, 0),
    P16("vae_36b.12h", 0x20000, 0x040000, 1),
    P16("vae_28b.9f",  0x20000, 0x080000, 0),
    P16("vae_33b.9h",  0x20000, 0x080000, 1),
    P16("vae_29b.10f", 0x20000, 0x0C0000, 0),
    P16("vae_34b.10h", 0x20000, 0x0C0000, 1),
};

inline constexpr RomSlot kVarthGfx[] =
{
    G64("va-5m.7a", 0x80000, 0x000000, 0),
    G64("va-7m.9a", 0x80000, 0x000000, 2),
    G64("va-1m.3a", 0x80000, 0x000000, 4),
    G64("va-3m.5a", 0x80000, 0x000000, 6),
};

inline constexpr GameLayout kVarth =
{
    "varth", "Varth: Operation Thunderstorm",
    kVarthProg, 0x100000,
    kVarthGfx,  0x200000,
};

} // namespace cps1::games
