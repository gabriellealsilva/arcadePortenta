#pragma once
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "Rom/RomDefs.h"
#include "core/Cps1System.h"

namespace cps1
{

class FilesystemRomSource;

/// @brief One running game on the host: owns the assembled ROM buffers and the
/// Cps1System that points into them.
///
/// Cps1System only holds non-owning spans (see Cps1System.h), so the buffers
/// must outlive it. Declaring them first here guarantees that through
/// reverse-order destruction.
class GameSession
{
public:
    /// @brief Assemble ROMs for @p game and reset the 68000.
    /// Any previous game is stopped first. @return @c false on failure
    /// (error already logged); the session is then inactive.
    bool start(FilesystemRomSource& src, const GameDescriptor& game);

    /// @brief Stop emulating and release the system (ROM buffers are kept).
    void stop() { system_.reset(); }

    bool active() const { return system_ != nullptr; }

    /// @brief Run one video frame. Prints a PC trace line every 60 frames if @p trace.
    void tick(bool trace);

    std::span<const uint16_t> framebuffer() const { return system_->framebuffer(); }
    uint32_t                  programCounter() const { return system_->programCounter(); }

private:
    std::vector<uint8_t>        progRom_;
    std::vector<uint8_t>        gfxRom_;
    std::unique_ptr<Cps1System> system_;
    int                         frames_ = 0;
};

/// @brief Find a game by folder id in a scanned list, or @c nullptr.
const GameDescriptor* findGame(std::span<const GameDescriptor> games, std::string_view id);

} // namespace cps1
