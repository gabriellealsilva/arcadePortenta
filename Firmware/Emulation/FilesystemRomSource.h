#pragma once
#include "Rom/IRomSource.h"
#include <filesystem>
#include <vector>

namespace cps1
{

/// @brief IRomSource backend that discovers game sets from the host filesystem.
///
/// Expected directory layout:
/// @code
///   <basePath>/
///     sf2/          <- one sub-directory per game
///     sf2ce/
///     megaman/
///     ...
/// @endcode
///
/// @ref scanImpl enumerates sub-directories without a hardcoded game database.
/// @ref readRomImpl opens files from disk on demand, with no caching.
class FilesystemRomSource : public IRomSource<FilesystemRomSource>
{
public:
    /// @brief Construct with the root directory that contains game sub-folders.
    /// @param basePath Absolute path to the @c ROMS/ directory.
    explicit FilesystemRomSource(const std::filesystem::path& basePath);

    /// @brief Enumerate all sub-directories of @c basePath as available games.
    void scanImpl();

    /// @brief Return the list built by the last @ref scanImpl call.
    std::span<const GameDescriptor> availableGamesImpl() const;

    /// @brief Read @c entry.size bytes from @c basePath/game.id/entry.filename
    ///        into @c dest.
    /// @return @c false if the file cannot be opened or @c dest is too small.
    bool readRomImpl(const GameDescriptor& game,
                     const RomEntry&       entry,
                     std::span<uint8_t>    dest);

private:
    std::filesystem::path      basePath;
    std::vector<GameDescriptor> available;
};

} // namespace cps1

static_assert(cps1::RomSourceImpl<cps1::FilesystemRomSource>,
              "FilesystemRomSource must satisfy the RomSourceImpl concept");
