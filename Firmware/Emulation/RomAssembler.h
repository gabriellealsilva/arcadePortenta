#pragma once
#include "Rom/RomDefs.h"
#include <cstdint>
#include <vector>

namespace cps1
{

class FilesystemRomSource;

/// @brief Assemble one game's program and GFX ROM images from individual
/// chip-dump files on disk, applying the byte interleave described by its
/// GameLayout (see Videogame/CPS-1/core/RomLayout.h).
///
/// This is the host-emulator-only half of ROM assembly: it reads files via
/// @ref FilesystemRomSource and owns the resulting buffers, since Cps1System
/// itself only accepts non-owning spans (see Cps1System.h). On hardware, no
/// equivalent runtime step is needed — flash mode assembles the same image
/// once ahead of time and writes it to QSPI; play mode then hands Cps1System
/// spans directly over the memory-mapped flash, with no copy.
///
/// @param progRom Output; resized to the game's program ROM size and filled.
/// @param gfxRom  Output; resized to the game's GFX ROM size and filled, or
///                left empty if the game has no GFX ROMs dumped.
/// @return @c false if @p game.id has no known GameLayout. A missing or
///         short individual chip file is logged and skipped, not fatal.
bool assembleRomImages(FilesystemRomSource& src, const GameDescriptor& game,
                       std::vector<uint8_t>& progRom, std::vector<uint8_t>& gfxRom);

} // namespace cps1
