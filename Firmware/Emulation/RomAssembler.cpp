#include "RomAssembler.h"
#include "FilesystemRomSource.h"
#include "Log.h"
#include "core/RomLayout.h"
#include <span>

namespace cps1
{

namespace
{

// ---------------------------------------------------------------------------
// Generic ROM slot loader — handles ROM_LOAD16_BYTE, ROM_LOAD64_WORD,
// ROM_LOAD64_BYTE, and ROM_LOAD16_WORD_SWAP (see core/RomLayout.h).
// ---------------------------------------------------------------------------
void loadSlots(std::span<const RomSlot> slots, FilesystemRomSource& src,
               const GameDescriptor& game, std::vector<uint8_t>& buf)
{
    for (const RomSlot& slot : slots)
    {
        std::vector<uint8_t> raw(slot.size);
        RomEntry entry;
        entry.filename = slot.filename;
        entry.size     = slot.size;
        entry.region   = slot.region;

        if (!src.readRomImpl(game, entry, raw))
        {
            LOG("[CPS1] Failed to read '%s/%s'", game.id, slot.filename);
            continue;
        }

        for (uint32_t i = 0; i < slot.size; ++i)
        {
            const uint32_t group       = i / slot.groupBytes;
            const uint32_t byteInGroup = i % slot.groupBytes;
            const uint32_t effByte     = slot.swap ? (slot.groupBytes - 1 - byteInGroup) : byteInGroup;
            buf[slot.destOffset + group * slot.stride + slot.destByte + effByte] = raw[i];
        }

        LOG("[CPS1] Loaded '%s'", slot.filename);
    }
}

} // namespace

bool assembleRomImages(FilesystemRomSource& src, const GameDescriptor& game,
                       std::vector<uint8_t>& progRom, std::vector<uint8_t>& gfxRom)
{
    const GameLayout* layout = findGameLayout(game.id);
    if (!layout)
    {
        LOG("[CPS1] No ROM layout for '%s'", game.id);
        return false;
    }

    // Program ROM — unprogrammed value is 0xFF (open bus / pulled high)
    progRom.assign(layout->progRomSize, 0xFF);
    loadSlots(layout->progSlots, src, game, progRom);

    // GFX ROM — missing tiles decode as transparent (0x00 bitplanes)
    gfxRom.clear();
    if (layout->gfxRomSize > 0)
    {
        gfxRom.assign(layout->gfxRomSize, 0x00);
        loadSlots(layout->gfxSlots, src, game, gfxRom);
    }

    return true;
}

} // namespace cps1
