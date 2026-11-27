#pragma once
#include <cstdint>

namespace cps1
{

/// @brief Per-title CPS-B custom-chip configuration (from MAME cps1_v.cpp,
/// @c CPS1config / @c cps1_config_table).
///
/// Games read a "CPS-B ID" register at boot as a board self-test and refuse
/// to continue (or hang) if it does not return the expected value. Later
/// boards add a 16×16→32 bit multiplier used as protection, and the C-board
/// can add extra input ports. Register offsets are byte offsets from 0x800140.
/// A value of -1 means "not present".
struct CpsBConfig
{
    int idAddr;       ///< Offset of the ID register
    int idValue;      ///< Value it returns
    int multFactor1;  ///< Multiplier operand registers (written by the game)
    int multFactor2;
    int multResultLo; ///< Product, low and high 16 bits (read by the game)
    int multResultHi;
    int layerCtrl;       ///< Layer-control register
    int priority[4];     ///< Priority-mask registers
    int paletteCtrl;     ///< Palette page enable register
    int layerEnable[5];  ///< Layer enable masks
    int in2Addr;         ///< Extra input port (C-board), 0 = none
    int in3Addr;         ///< Player 4 input port, 0 = none
};

/// @brief CPS-B configuration for a game by MAME set id.
/// @return A pointer to a static table entry, or @c nullptr if unknown.
const CpsBConfig* findCpsBConfig(const char* gameId);

} // namespace cps1
