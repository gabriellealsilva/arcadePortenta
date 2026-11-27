#pragma once
#include <filesystem>

namespace cps1
{

/// @brief Parsed host-emulator command line.
///
/// Usage: Emulator-CPS-1 [romsDir]
struct Options
{
    std::filesystem::path romsPath;
};

/// @brief Parse argv into @p out. @p out.romsPath must be pre-set to the default.
/// @return @c false (after printing a message to stderr) on invalid usage.
bool parseCommandLine(int argc, char* argv[], Options& out);

} // namespace cps1
