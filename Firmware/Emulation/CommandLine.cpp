#include "CommandLine.h"

#include <cstdio>
#include <string_view>

namespace cps1
{

bool parseCommandLine(int argc, char* argv[], Options& out)
{
    for (int i = 1; i < argc; ++i)
    {
        const std::string_view a = argv[i];

        if (!a.starts_with("--"))
            out.romsPath = argv[i];
        else
        {
            std::fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return false;
        }
    }
    return true;
}

} // namespace cps1
