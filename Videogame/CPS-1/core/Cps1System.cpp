#include "core/Cps1System.h"
#include "Log.h"
#include <algorithm>
#include <cstring>

extern "C"
{
#include <m68k.h>
}

namespace cps1
{

// ---------------------------------------------------------------------------
// Global Musashi dispatch — one instance at a time.
// ---------------------------------------------------------------------------
static Cps1System* gSystem = nullptr;

} // namespace cps1

extern "C"
{
    unsigned int m68k_read_memory_8 (unsigned int addr) { return cps1::gSystem->read8(addr); }
    unsigned int m68k_read_memory_16(unsigned int addr) { return cps1::gSystem->read16(addr); }
    unsigned int m68k_read_memory_32(unsigned int addr) { return cps1::gSystem->read32(addr); }
    void m68k_write_memory_8 (unsigned int addr, unsigned int val) { cps1::gSystem->write8(addr, val); }
    void m68k_write_memory_16(unsigned int addr, unsigned int val) { cps1::gSystem->write16(addr, val); }
    void m68k_write_memory_32(unsigned int addr, unsigned int val) { cps1::gSystem->write32(addr, val); }

    /// Called when the 68k acknowledges an interrupt.
    /// Clears the IRQ line immediately so it is not re-asserted next frame.
    static int intAck(int /*level*/)
    {
        m68k_set_irq(M68K_IRQ_NONE);
        return M68K_INT_ACK_AUTOVECTOR;
    }
}

namespace cps1
{

// ---------------------------------------------------------------------------
// CPS-1 address space
// ---------------------------------------------------------------------------
static constexpr uint32_t PROG_ROM_END  = 0x3FFFFF;
static constexpr uint32_t GFX_RAM_BASE  = 0x800000;
static constexpr uint32_t GFX_RAM_END   = 0x87FFFF;
static constexpr uint32_t PALETTE_BASE  = 0x900000;
static constexpr uint32_t PALETTE_END   = 0x91FFFF;
static constexpr uint32_t WORK_RAM_BASE = 0xFF0000;

// 10 MHz 68000, ~59.637 Hz display
static constexpr int CYCLES_PER_FRAME = 167'694;

// ---------------------------------------------------------------------------
// CPS-1 palette entry: XBBBBBGGGGGRRRRR → RGB565 RRRRRGGGGGGBBBBB
// ---------------------------------------------------------------------------
static uint16_t palToRgb565(uint16_t p)
{
    const uint32_t r  = (p >>  0) & 0x1F;
    const uint32_t g  = (p >>  5) & 0x1F;
    const uint32_t b  = (p >> 10) & 0x1F;
    const uint32_t g6 = (g << 1) | (g >> 4); // 5-bit → 6-bit
    return static_cast<uint16_t>((r << 11) | (g6 << 5) | b);
}

// ---------------------------------------------------------------------------
// 8×8 tile pixel decode from the assembled GFX ROM buffer.
//
// Layout after ROM_LOAD64_WORD (stride=8, groupBytes=2):
//   Each 8-byte group covers 2 consecutive tile rows across 4 bitplanes.
//   Within the group:
//     bytes 0,1 = bitplane 0 (rows 2k, 2k+1)
//     bytes 2,3 = bitplane 1
//     bytes 4,5 = bitplane 2
//     bytes 6,7 = bitplane 3
//   Within each bitplane byte: bit 7 = leftmost pixel (MSB first).
//
//   Tile N = 32 bytes starting at N*32.
//   Row pair k (rows 2k, 2k+1) = 8-byte group at offset k*8 within the tile.
// ---------------------------------------------------------------------------
static uint8_t getPixel(std::span<const uint8_t> gfx,
                         uint32_t tile, int row, int col)
{
    const uint32_t base = tile * 32 + static_cast<uint32_t>(row / 2) * 8;
    const uint32_t rb   = static_cast<uint32_t>(row & 1); // row byte (0 or 1)
    const int      bit  = 7 - col;                         // MSB = leftmost pixel

    const uint8_t bp0 = (gfx[base + 0 + rb] >> bit) & 1;
    const uint8_t bp1 = (gfx[base + 2 + rb] >> bit) & 1;
    const uint8_t bp2 = (gfx[base + 4 + rb] >> bit) & 1;
    const uint8_t bp3 = (gfx[base + 6 + rb] >> bit) & 1;
    return static_cast<uint8_t>(bp0 | (bp1 << 1) | (bp2 << 2) | (bp3 << 3));
}

// ---------------------------------------------------------------------------
// Cps1System
// ---------------------------------------------------------------------------

Cps1System::Cps1System()  = default;

Cps1System::~Cps1System()
{
    if (gSystem == this)
        gSystem = nullptr;
}

bool Cps1System::load(std::span<const uint8_t> progRom, std::span<const uint8_t> gfxRom)
{
    if (progRom.empty())
    {
        LOG("[CPS1] Refusing to load: empty program ROM");
        return false;
    }

    this->progRom = progRom;
    this->gfxRom  = gfxRom;

    std::memset(workRam,    0, sizeof(workRam));
    std::memset(gfxRam,     0, sizeof(gfxRam));
    std::memset(paletteRam, 0, sizeof(paletteRam));
    std::memset(cpsARegs,   0, sizeof(cpsARegs));

    gSystem = this;
    m68k_set_cpu_type(M68K_CPU_TYPE_68000);
    m68k_init();
    m68k_set_int_ack_callback(intAck);
    m68k_pulse_reset();

    LOG("[CPS1] 68000 reset — PC=0x%06X SP=0x%06X",
        m68k_get_reg(nullptr, M68K_REG_PC),
        m68k_get_reg(nullptr, M68K_REG_SP));

    return true;
}

void Cps1System::tick()
{
    m68k_execute(CYCLES_PER_FRAME);
    m68k_set_irq(2); // VBlank, level 2, cleared by intAck when acknowledged
    renderFrame();
}

// ---------------------------------------------------------------------------
// Frame renderer: background colour + Scroll1 (8×8 tile layer).
//
// Scroll1 tilemap lives at the start of GFX RAM (gfxRam[0]).
// Tilemap is logically 64 tiles wide × 32 tiles tall (512×256 px).
// The visible window is 48×28 tiles (384×224 px screen).
//
// Each 16-bit tilemap entry (big-endian):
//   bits [11:0]  — tile code
//   bits [15:12] — colour (palette select, 0–15)
//
// Palette RAM entry (16-bit, big-endian):
//   XBBBBBGGGGGRRRRR
//
// Pixel nibble 0 is transparent (shows background colour behind).
// ---------------------------------------------------------------------------
void Cps1System::renderFrame()
{
    // Helper: read a big-endian 16-bit palette entry.
    const auto readPal = [&](uint32_t idx) -> uint16_t
    {
        const uint32_t off = idx * 2;
        return static_cast<uint16_t>((paletteRam[off] << 8) | paletteRam[off + 1]);
    };

    // Background colour — CPS-1 uses palette entry 0x0FFF as the clear pen.
    const uint16_t bgRgb = palToRgb565(readPal(0x0FFF));
    std::fill(std::begin(fb), std::end(fb), bgRgb);

    if (gfxRom.empty()) return;

    const uint32_t maxTile = static_cast<uint32_t>(gfxRom.size()) / 32;

    // CPS-A register indices (base 0xC00000):
    //   [0] Scroll1 X   [1] Scroll1 Y   [8] Scroll1 base   [11] OBJ base
    const uint32_t sc1Base = static_cast<uint32_t>(cpsARegs[8] & 0x7FFF) * 2
                              % sizeof(gfxRam);
    const int      scrollX = static_cast<int16_t>(cpsARegs[0]);
    const int      scrollY = static_cast<int16_t>(cpsARegs[1]);

    for (int sy = 0; sy < HEIGHT; ++sy)
    {
        for (int sx = 0; sx < WIDTH; ++sx)
        {
            // Apply scroll then wrap within the 512×256 tilemap.
            const int mx = (sx + scrollX) & 511; // tilemap is 512 px wide
            const int my = (sy + scrollY) & 255; // tilemap is 256 px tall
            const int tx = mx / 8;               // tile column (0–63)
            const int ty = my / 8;               // tile row    (0–31)

            // Tilemap entry — big-endian 16-bit word in gfxRam
            const uint32_t mapOff = sc1Base + static_cast<uint32_t>(ty * 64 + tx) * 2;
            if (mapOff + 1 >= sizeof(gfxRam)) continue;
            const uint16_t entry  = static_cast<uint16_t>(
                (gfxRam[mapOff] << 8) | gfxRam[mapOff + 1]);

            const uint32_t tileCode = entry & 0x0FFF;
            const uint32_t color    = (entry >> 12) & 0xF;

            if (tileCode == 0 || tileCode >= maxTile)
                continue; // blank tile or out-of-range — keep background

            const uint8_t nibble = getPixel(gfxRom, tileCode, my & 7, mx & 7);
            if (nibble == 0)
                continue; // transparent pixel — keep background

            fb[sy * WIDTH + sx] = palToRgb565(readPal(color * 16 + nibble));
        }
    }

    // -----------------------------------------------------------------------
    // OBJ (sprite) layer
    //
    // List base: CPS-A register 3 (addr 0xC00016) = word index into gfxRam.
    // Each entry = 4 big-endian 16-bit words (8 bytes):
    //   [0] tile code
    //   [1] bits[4:0]=palette  bit[5]=flipX  bit[6]=flipY
    //   [2] X coordinate (0-511, screen origin at X=64)
    //   [3] Y coordinate (0-511, screen origin at Y=16) — bit 9 set = end-of-list
    //
    // Each sprite is 16×16 px: a 2×2 block of consecutive 8×8 tiles.
    // -----------------------------------------------------------------------
    {
        const uint32_t objBase = static_cast<uint32_t>(cpsARegs[11]) * 2
                                  % sizeof(gfxRam);

        auto rdw = [&](uint32_t off) -> uint16_t
        {
            if (off + 1 >= sizeof(gfxRam)) return 0;
            return static_cast<uint16_t>((gfxRam[off] << 8) | gfxRam[off + 1]);
        };

        for (int i = 0; i < 256; ++i)
        {
            const uint32_t e    = objBase + static_cast<uint32_t>(i) * 8;
            const uint16_t code = rdw(e + 0);
            const uint16_t attr = rdw(e + 2);
            const uint16_t rawX = rdw(e + 4);
            const uint16_t rawY = rdw(e + 6);

            if (rawY & 0x200) break;               // end-of-list marker
            if (!code && !rawX && !rawY) break;    // all-zero sentinel

            const uint32_t pal   = attr & 0x1F;
            const bool     flipX = (attr >> 5) & 1;
            const bool     flipY = (attr >> 6) & 1;
            const int      spX   = (rawX & 0x1FF) - 64;
            const int      spY   = (rawY & 0x1FF) - 16;

            for (int ty = 0; ty < 2; ++ty)
            {
                for (int tx = 0; tx < 2; ++tx)
                {
                    const uint32_t subTile = code + static_cast<uint32_t>(
                        (flipY ? 1 - ty : ty) * 2 + (flipX ? 1 - tx : tx));
                    if (subTile >= maxTile) continue;

                    for (int py = 0; py < 8; ++py)
                    {
                        const int dstY = spY + ty * 8 + (flipY ? 7 - py : py);
                        if (dstY < 0 || dstY >= HEIGHT) continue;

                        for (int px = 0; px < 8; ++px)
                        {
                            const int dstX = spX + tx * 8 + (flipX ? 7 - px : px);
                            if (dstX < 0 || dstX >= WIDTH) continue;

                            const uint8_t nibble = getPixel(gfxRom, subTile, py, px);
                            if (nibble == 0) continue;

                            fb[dstY * WIDTH + dstX] =
                                palToRgb565(readPal(pal * 16 + nibble));
                        }
                    }
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Memory read
// ---------------------------------------------------------------------------
uint32_t Cps1System::read8(uint32_t addr) const
{
    addr &= 0xFFFFFF;

    if (addr <= PROG_ROM_END)
        return (addr < progRom.size()) ? progRom[addr] : 0xFF;

    if (addr >= GFX_RAM_BASE && addr <= GFX_RAM_END)
        return gfxRam[addr - GFX_RAM_BASE];

    if (addr >= PALETTE_BASE && addr <= PALETTE_END)
        return paletteRam[addr - PALETTE_BASE];

    if (addr >= WORK_RAM_BASE)
        return workRam[addr & 0xFFFF];

    return 0xFF; // open bus / unpressed inputs / DIP off
}

uint32_t Cps1System::read16(uint32_t addr) const
{
    addr &= 0xFFFFFF;
    return (read8(addr) << 8) | read8(addr + 1);
}

uint32_t Cps1System::read32(uint32_t addr) const
{
    return (read16(addr) << 16) | read16(addr + 2);
}

// ---------------------------------------------------------------------------
// Memory write
// ---------------------------------------------------------------------------
void Cps1System::write8(uint32_t addr, uint32_t val)
{
    addr &= 0xFFFFFF;

    if (addr >= GFX_RAM_BASE && addr <= GFX_RAM_END)
        { gfxRam[addr - GFX_RAM_BASE] = static_cast<uint8_t>(val); return; }

    if (addr >= PALETTE_BASE && addr <= PALETTE_END)
        { paletteRam[addr - PALETTE_BASE] = static_cast<uint8_t>(val); return; }

    if (addr >= WORK_RAM_BASE)
        { workRam[addr & 0xFFFF] = static_cast<uint8_t>(val); return; }

    // ROM, I/O, sound latch — silently ignored for now
}

void Cps1System::write16(uint32_t addr, uint32_t val)
{
    addr &= 0xFFFFFF;

    // CPS-A write-only registers (0xC00000–0xC0003E).
    // Scroll X/Y at 0xC00000–0xC0000E, tilemap/OBJ bases at 0xC00010–0xC00016.
    if (addr >= 0xC00000 && addr < 0xC00040)
    {
        const uint32_t idx = (addr - 0xC00000) / 2;
        if (idx < 32)
            cpsARegs[idx] = static_cast<uint16_t>(val);
        return;
    }

    write8(addr,     (val >> 8) & 0xFF);
    write8(addr + 1,  val       & 0xFF);
}

void Cps1System::write32(uint32_t addr, uint32_t val)
{
    write16(addr,     (val >> 16) & 0xFFFF);
    write16(addr + 2,  val        & 0xFFFF);
}

} // namespace cps1
