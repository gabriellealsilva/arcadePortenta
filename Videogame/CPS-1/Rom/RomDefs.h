#pragma once
#include <cstdint>
#include <string_view>

namespace cps1
{

/// @brief Region of the CPS-1 PCB that a ROM chip occupies.
enum class RomRegion : uint8_t
{
    Program68k,   ///< 68000 main CPU program ROM
    ProgramZ80,   ///< Z80 audio CPU program ROM
    Gfx,          ///< Graphics tile data
    AudioSamples, ///< OKI MSM6295 PCM sample data
};

/// @brief Identifies one ROM file within a game set.
///
/// Passed to @ref IRomSource::readRom to specify which file to load.
/// No dynamic allocation: on firmware @c filename is a @c string_view into
/// QSPI flash; on host it is a short-lived view valid only for the duration
/// of the @c readRom call.
struct RomEntry
{
    std::string_view filename; ///< File name (e.g. @c "sf2e_28g.9e")
    uint32_t         size;     ///< Expected file size in bytes
    RomRegion        region;   ///< Which hardware region this ROM feeds
};

/// @brief Lean game identity record.
///
/// Fixed-size char arrays keep this trivially copyable and heap-free,
/// suitable for both host and bare-metal backends.
/// ROM file lists are not stored here — they are enumerated on demand
/// at load time by the concrete backend.
struct GameDescriptor
{
    char id[32]    = {}; ///< Folder name under @c ROMS/ (e.g. @c "sf2")
    char title[64] = {}; ///< Human-readable display name
};

} // namespace cps1
