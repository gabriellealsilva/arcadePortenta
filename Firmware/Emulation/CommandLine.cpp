#include "CommandLine.h"

#include <cstdio>
#include <cstdlib>
#include <string_view>

namespace cps1
{

bool parseCommandLine(int argc, char* argv[], Options& out)
{
    for (int i = 1; i < argc; ++i)
    {
        const std::string_view a = argv[i];
        const bool hasValue = (i + 1 < argc);

        if (a == "--game" && hasValue)
            out.gameId = argv[++i];
        else if (a == "--snapshot" && hasValue)
            out.snapshotPath = argv[++i];
        else if (a == "--frames" && hasValue)
            out.frames = std::atoi(argv[++i]);
        else if (a == "--trace")
            out.trace = true;
        else if (!a.starts_with("--"))
            out.romsPath = argv[i];
        else
        {
            std::fprintf(stderr, "Unknown or incomplete option: %s\n", argv[i]);
            return false;
        }
    }

    if (out.headless() && out.gameId.empty())
    {
        std::fprintf(stderr, "--snapshot requires --game <id>\n");
        return false;
    }
    return true;
}

} // namespace cps1
