#include "FilesystemRomSource.h"
#include "Log.h"
#include <cstring>
#include <fstream>

namespace cps1
{

FilesystemRomSource::FilesystemRomSource(const std::filesystem::path& basePath)
    : basePath(basePath)
{}

void FilesystemRomSource::scanImpl()
{
    available.clear();

    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(basePath, ec))
    {
        if (!entry.is_directory(ec))
            continue;

        const std::string name = entry.path().filename().string();
        GameDescriptor gd{};
        std::strncpy(gd.id,    name.c_str(), sizeof(gd.id)    - 1);
        std::strncpy(gd.title, name.c_str(), sizeof(gd.title) - 1);
        available.push_back(gd);
        LOG("[ROM] found: %s", name.c_str());
    }

    LOG("[ROM] scan complete: %d game(s) found", static_cast<int>(available.size()));
}

std::span<const GameDescriptor> FilesystemRomSource::availableGamesImpl() const
{
    return available;
}

bool FilesystemRomSource::readRomImpl(const GameDescriptor& game,
                                      const RomEntry&       entry,
                                      std::span<uint8_t>    dest)
{
    if (dest.size() < entry.size)
        return false;

    const auto path = basePath / game.id / entry.filename;
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return false;

    return static_cast<bool>(
        f.read(reinterpret_cast<char*>(dest.data()),
               static_cast<std::streamsize>(entry.size)));
}

} // namespace cps1
