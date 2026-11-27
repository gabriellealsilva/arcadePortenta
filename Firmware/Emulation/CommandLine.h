#pragma once
#include <filesystem>
#include <string>

namespace cps1
{

/// @brief Parsed host-emulator command line.
///
/// Usage: Emulator-CPS-1 [romsDir] [--game <id>] [--frames <n>]
///                       [--snapshot <file.png>] [--trace]
///   --game      boot this game directly, skipping the menu
///   --snapshot  headless: run --frames frames (default 600), save the
///               framebuffer as PNG and exit (needs --game)
///   --trace     print the 68000 PC once per 60 frames to stderr
struct Options
{
    std::filesystem::path romsPath;
    std::string           gameId;       ///< empty = show the menu
    std::string           snapshotPath; ///< empty = interactive run
    int                   frames = 600; ///< headless frame count
    bool                  trace  = false;

    bool headless() const { return !snapshotPath.empty(); }
};

/// @brief Parse argv into @p out. @p out.romsPath must be pre-set to the default.
/// @return @c false (after printing a message to stderr) on invalid usage.
bool parseCommandLine(int argc, char* argv[], Options& out);

} // namespace cps1
