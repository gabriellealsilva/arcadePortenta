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
static Cps1System *gSystem = nullptr;

} // namespace cps1

extern "C"
{
    unsigned int m68k_read_memory_8(unsigned int addr)
    {
        return cps1::gSystem->read8(addr);
    }
    unsigned int m68k_read_memory_16(unsigned int addr)
    {
        return cps1::gSystem->read16(addr);
    }
    unsigned int m68k_read_memory_32(unsigned int addr)
    {
        return cps1::gSystem->read32(addr);
    }
    void m68k_write_memory_8(unsigned int addr, unsigned int val)
    {
        cps1::gSystem->write8(addr, val);
    }
    void m68k_write_memory_16(unsigned int addr, unsigned int val)
    {
        cps1::gSystem->write16(addr, val);
    }
    void m68k_write_memory_32(unsigned int addr, unsigned int val)
    {
        cps1::gSystem->write32(addr, val);
    }

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
// CPS-1 address space (MAME cps1.cpp, cps_state::main_map)
//
//   000000–3FFFFF  program ROM
//   800000–800007  player inputs (IN1)            read
//   800018–80001F  system inputs + DIP switches   read
//   800030–800037  coin control                   write
//   800100–80013F  CPS-A registers                write
//   800140–80017F  CPS-B registers / ID / mult    read/write
//   800180–80018F  sound latches                  write
//   900000–92FFFF  GFX RAM (scroll, OBJ, palette source)
//   FF0000–FFFFFF  work RAM
// ---------------------------------------------------------------------------
static constexpr uint32_t PROG_ROM_END = 0x3FFFFF;
static constexpr uint32_t IO_BASE = 0x800000;
static constexpr uint32_t IO_END = 0x8001FF;
static constexpr uint32_t CPS_A_BASE = 0x800100;
static constexpr uint32_t CPS_B_BASE = 0x800140;
static constexpr uint32_t GFX_RAM_BASE = 0x900000;
static constexpr uint32_t GFX_RAM_END = 0x92FFFF;
static constexpr uint32_t WORK_RAM_BASE = 0xFF0000;

// Palette pages are laid out as 6 × 0x200 entries; the first pen of each
// layer family inside that range (MAME cps1_v.cpp gfx_element bases).
static constexpr uint32_t PEN_OBJ = 0x000;
static constexpr uint32_t PEN_SCROLL1 = 0x200;
static constexpr uint32_t PEN_BACKGROUND = 0xBFF; // clear pen

// 10 MHz 68000, ~59.637 Hz display
static constexpr int CYCLES_PER_FRAME = 167'694;

// ---------------------------------------------------------------------------
// CPS-1 palette entry: BBBB RRRR GGGG BBBB (brightness, R, G, B nibbles)
// → RGB565. Brightness 0 dims to 1/3, per MAME cps1_build_palette().
// ---------------------------------------------------------------------------
static uint16_t cpsColorToRgb565(uint16_t p)
{
    const uint32_t bright = 0x0F + ((p >> 12) << 1);
    const uint32_t r = ((p >> 8) & 0xF) * 0x11 * bright / 0x2D;
    const uint32_t g = ((p >> 4) & 0xF) * 0x11 * bright / 0x2D;
    const uint32_t b = ((p >> 0) & 0xF) * 0x11 * bright / 0x2D;
    return static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
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
//
// NOTE: tile codes are used as raw ROM indices; the per-game GFX bank mapper
// (MAME gfx_range tables) is not implemented yet.
// ---------------------------------------------------------------------------
static uint8_t getPixel(std::span<const uint8_t> gfx, uint32_t tile, int row, int col)
{
    const uint32_t base = tile * 32 + static_cast<uint32_t>(row / 2) * 8;
    const uint32_t rb = static_cast<uint32_t>(row & 1); // row byte (0 or 1)
    const int bit = 7 - col;                            // MSB = leftmost pixel

    const uint8_t bp0 = (gfx[base + 0 + rb] >> bit) & 1;
    const uint8_t bp1 = (gfx[base + 2 + rb] >> bit) & 1;
    const uint8_t bp2 = (gfx[base + 4 + rb] >> bit) & 1;
    const uint8_t bp3 = (gfx[base + 6 + rb] >> bit) & 1;
    return static_cast<uint8_t>(bp0 | (bp1 << 1) | (bp2 << 2) | (bp3 << 3));
}

// ---------------------------------------------------------------------------
// Cps1System
// ---------------------------------------------------------------------------

Cps1System::Cps1System() = default;

Cps1System::~Cps1System()
{
    if (gSystem == this)
        gSystem = nullptr;
}

bool Cps1System::load(std::span<const uint8_t> progRom, std::span<const uint8_t> gfxRom,
                      const CpsBConfig &cpsb)
{
    if (progRom.empty())
    {
        LOG("[CPS1] Refusing to load: empty program ROM");
        return false;
    }

    this->progRom = progRom;
    this->gfxRom = gfxRom;
    this->cpsb = &cpsb;

    std::memset(workRam, 0, sizeof(workRam));
    std::memset(gfxRam, 0, sizeof(gfxRam));
    std::memset(palette, 0, sizeof(palette));
    std::memset(cpsARegs, 0, sizeof(cpsARegs));
    std::memset(cpsBRegs, 0, sizeof(cpsBRegs));

    gSystem = this;
    m68k_set_cpu_type(M68K_CPU_TYPE_68000);
    m68k_init();
    m68k_set_int_ack_callback(intAck);
    m68k_pulse_reset();

    LOG("[CPS1] 68000 reset — PC=0x%06X SP=0x%06X", m68k_get_reg(nullptr, M68K_REG_PC),
        m68k_get_reg(nullptr, M68K_REG_SP));

    return true;
}

uint32_t Cps1System::programCounter() const
{
    return m68k_get_reg(nullptr, M68K_REG_PC);
}

void Cps1System::tick()
{
    m68k_execute(CYCLES_PER_FRAME);
    m68k_set_irq(2); // VBlank, level 2, cleared by intAck when acknowledged
    renderFrame();
}

// ---------------------------------------------------------------------------
// Palette: the CPU writes colours into GFX RAM; the CPS-B copies them into the
// real palette RAM right after the palette-base register is written (MAME
// cps1_build_palette). Only pages enabled in the CPS-B palette-control
// register are copied.
// ---------------------------------------------------------------------------
uint16_t Cps1System::gfxRamWord(uint32_t byteOff) const
{
    byteOff %= sizeof(gfxRam);
    return static_cast<uint16_t>((gfxRam[byteOff] << 8) | gfxRam[(byteOff + 1) % sizeof(gfxRam)]);
}

void Cps1System::buildPalette()
{
    // Base is the register value × 256, aligned to a 0x400 palette boundary.
    const uint32_t base = (static_cast<uint32_t>(cpsARegs[PaletteBase]) * 256u) & ~0x3FFu & 0x3FFFFu;
    const uint16_t ctrl = (cpsb && cpsb->paletteCtrl >= 0) ? cpsBRegs[cpsb->paletteCtrl / 2] : 0;

    uint32_t src = base;
    bool copied = false;
    for (int page = 0; page < 6; ++page)
    {
        if (ctrl & (1u << page))
        {
            for (uint32_t i = 0; i < 0x200; ++i, src += 2)
                palette[page * 0x200 + i] = cpsColorToRgb565(gfxRamWord(src));
            copied = true;
        }
        else if (copied)
        {
            src += 0x400; // skip the page in GFX RAM once we have copied one
        }
    }
}

// ---------------------------------------------------------------------------
// Frame renderer (provisional): background pen + Scroll1 (8×8) + OBJ.
//
// Register/layout details follow MAME cps1_v.cpp. Missing for now: scroll 2
// and 3, layer priorities, rowscroll, screen flip, the GFX bank mapper and
// the correct CPS-1 tile bit layout. So this is a debugging view, not a
// faithful picture.
// ---------------------------------------------------------------------------
void Cps1System::renderFrame()
{
    std::fill(std::begin(fb), std::end(fb), palette[PEN_BACKGROUND]);

    if (gfxRom.empty())
        return;

    const uint32_t maxTile = static_cast<uint32_t>(gfxRom.size()) / 32;

    // Scroll 1: 64×32 tiles of 8×8 px, 4 bytes per entry (code, attribute).
    // The tilemap RAM must start on a 0x4000 boundary.
    const uint32_t sc1Base = (static_cast<uint32_t>(cpsARegs[Scroll1Base]) * 256u) & ~0x3FFFu;
    const int scrollX = static_cast<int16_t>(cpsARegs[Scroll1X]);
    const int scrollY = static_cast<int16_t>(cpsARegs[Scroll1Y]);

    for (int sy = 0; sy < HEIGHT; ++sy)
    {
        for (int sx = 0; sx < WIDTH; ++sx)
        {
            const int mx = (sx + scrollX) & 511;
            const int my = (sy + scrollY) & 255;
            const int tx = mx / 8;
            const int ty = my / 8;

            const uint32_t entry = sc1Base + static_cast<uint32_t>(ty * 64 + tx) * 4;
            const uint32_t tileCode = gfxRamWord(entry);
            const uint16_t attr = gfxRamWord(entry + 2);

            if (tileCode == 0 || tileCode >= maxTile)
                continue;

            const int px = (attr & 0x20) ? 7 - (mx & 7) : (mx & 7);
            const int py = (attr & 0x40) ? 7 - (my & 7) : (my & 7);
            const uint8_t nibble = getPixel(gfxRom, tileCode, py, px);
            if (nibble == 0)
                continue;

            fb[sy * WIDTH + sx] = palette[PEN_SCROLL1 + (attr & 0x1F) * 16 + nibble];
        }
    }

    // -----------------------------------------------------------------------
    // OBJ (sprite) layer. 8 bytes per entry, big-endian words:
    //   [0] X   [1] Y   [2] tile code   [3] attribute
    //   attribute: bits 0–4 palette, bit 5 flipX, bit 6 flipY, bits 8–11 nx,
    //   bits 12–15 ny (block size − 1). 0xFF00 in the attribute ends the list.
    // Each sprite is 16×16 px: a 2×2 block of consecutive 8×8 tiles (block
    // sizes larger than 1×1 are not handled yet).
    // -----------------------------------------------------------------------
    {
        const uint32_t objBase = (static_cast<uint32_t>(cpsARegs[ObjBase]) * 256u) & ~0x7FFu;

        for (int i = 0; i < 256; ++i)
        {
            const uint32_t e = objBase + static_cast<uint32_t>(i) * 8;
            const uint16_t rawX = gfxRamWord(e + 0);
            const uint16_t rawY = gfxRamWord(e + 2);
            const uint16_t code = gfxRamWord(e + 4);
            const uint16_t attr = gfxRamWord(e + 6);

            if (attr == 0xFF00)
                break; // end-of-list marker

            const uint32_t pal = attr & 0x1F;
            const bool flipX = (attr >> 5) & 1;
            const bool flipY = (attr >> 6) & 1;
            const int spX = (rawX & 0x1FF) - 64;
            const int spY = (rawY & 0x1FF) - 16;

            for (int ty = 0; ty < 2; ++ty)
            {
                for (int tx = 0; tx < 2; ++tx)
                {
                    const uint32_t subTile =
                        code +
                        static_cast<uint32_t>((flipY ? 1 - ty : ty) * 2 + (flipX ? 1 - tx : tx));
                    if (subTile >= maxTile)
                        continue;

                    for (int py = 0; py < 8; ++py)
                    {
                        const int dstY = spY + ty * 8 + (flipY ? 7 - py : py);
                        if (dstY < 0 || dstY >= HEIGHT)
                            continue;

                        for (int px = 0; px < 8; ++px)
                        {
                            const int dstX = spX + tx * 8 + (flipX ? 7 - px : px);
                            if (dstX < 0 || dstX >= WIDTH)
                                continue;

                            const uint8_t nibble = getPixel(gfxRom, subTile, py, px);
                            if (nibble == 0)
                                continue;

                            fb[dstY * WIDTH + dstX] = palette[PEN_OBJ + pal * 16 + nibble];
                        }
                    }
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// I/O region 0x800000–0x8001FF — 16-bit reads
// ---------------------------------------------------------------------------
uint32_t Cps1System::readIo16(uint32_t addr) const
{
    // Inputs are active low and nothing is pressed: all ones.
    // 800000–800007 player ports; 800018–80001F: coin/start + 3 DIP banks
    // (MAME returns (value << 8) | 0xFF).
    if (addr < 0x800008 || (addr >= 0x800018 && addr < 0x800020))
        return 0xFFFF;

    if (addr >= CPS_B_BASE && addr < CPS_B_BASE + sizeof(cpsBRegs))
    {
        const int off = static_cast<int>(addr - CPS_B_BASE) & ~1;

        if (cpsb)
        {
            // Board self-test ID register
            if (cpsb->idAddr >= 0 && off == cpsb->idAddr)
                return static_cast<uint32_t>(cpsb->idValue) & 0xFFFF;

            // 16×16 → 32 bit multiplier protection
            if (cpsb->multResultLo >= 0 && off == cpsb->multResultLo)
                return (static_cast<uint32_t>(cpsBRegs[cpsb->multFactor1 / 2]) *
                        cpsBRegs[cpsb->multFactor2 / 2]) & 0xFFFF;
            if (cpsb->multResultHi >= 0 && off == cpsb->multResultHi)
                return (static_cast<uint32_t>(cpsBRegs[cpsb->multFactor1 / 2]) *
                        cpsBRegs[cpsb->multFactor2 / 2]) >> 16;

            // Extra C-board input ports: nothing pressed
            if ((cpsb->in2Addr != 0 && off == cpsb->in2Addr) ||
                (cpsb->in3Addr != 0 && off == cpsb->in3Addr))
                return 0xFFFF;
        }
        return 0xFFFF; // any other CPS-B read returns all ones in MAME
    }

    return 0xFFFF; // open bus
}

void Cps1System::writeIo16(uint32_t addr, uint32_t val)
{
    if (addr >= CPS_A_BASE && addr < CPS_A_BASE + sizeof(cpsARegs))
    {
        const uint32_t idx = (addr - CPS_A_BASE) / 2;
        cpsARegs[idx] = static_cast<uint16_t>(val);
        if (idx == PaletteBase)
            buildPalette();
        return;
    }

    if (addr >= CPS_B_BASE && addr < CPS_B_BASE + sizeof(cpsBRegs))
    {
        cpsBRegs[(addr - CPS_B_BASE) / 2] = static_cast<uint16_t>(val);
        return;
    }

    // Coin control (800030), sound latches (800180/800188): ignored for now.
}

// ---------------------------------------------------------------------------
// Memory read
// ---------------------------------------------------------------------------
uint32_t Cps1System::read8(uint32_t addr) const
{
    addr &= 0xFFFFFF;

    if (addr <= PROG_ROM_END)
        return (addr < progRom.size()) ? progRom[addr] : 0xFF;

    if (addr >= IO_BASE && addr <= IO_END)
    {
        const uint32_t w = readIo16(addr & ~1u);
        return (addr & 1) ? (w & 0xFF) : (w >> 8);
    }

    if (addr >= GFX_RAM_BASE && addr <= GFX_RAM_END)
        return gfxRam[addr - GFX_RAM_BASE];

    if (addr >= WORK_RAM_BASE)
        return workRam[addr & 0xFFFF];

    return 0xFF; // open bus
}

uint32_t Cps1System::read16(uint32_t addr) const
{
    addr &= 0xFFFFFF;

    if (addr >= IO_BASE && addr <= IO_END)
        return readIo16(addr);

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

    if (addr >= IO_BASE && addr <= IO_END)
    {
        // Byte write to a 16-bit register: merge into the current word.
        const uint32_t even = addr & ~1u;
        uint32_t cur = 0;
        if (even >= CPS_A_BASE && even < CPS_A_BASE + sizeof(cpsARegs))
            cur = cpsARegs[(even - CPS_A_BASE) / 2];
        else if (even >= CPS_B_BASE && even < CPS_B_BASE + sizeof(cpsBRegs))
            cur = cpsBRegs[(even - CPS_B_BASE) / 2];
        cur = (addr & 1) ? ((cur & 0xFF00) | (val & 0xFF)) : ((cur & 0x00FF) | ((val & 0xFF) << 8));
        writeIo16(even, cur);
        return;
    }

    if (addr >= GFX_RAM_BASE && addr <= GFX_RAM_END)
    {
        gfxRam[addr - GFX_RAM_BASE] = static_cast<uint8_t>(val);
        return;
    }

    if (addr >= WORK_RAM_BASE)
    {
        workRam[addr & 0xFFFF] = static_cast<uint8_t>(val);
        return;
    }

    // ROM and unmapped space — silently ignored
}

void Cps1System::write16(uint32_t addr, uint32_t val)
{
    addr &= 0xFFFFFF;

    if (addr >= IO_BASE && addr <= IO_END)
    {
        writeIo16(addr, val);
        return;
    }

    write8(addr, (val >> 8) & 0xFF);
    write8(addr + 1, val & 0xFF);
}

void Cps1System::write32(uint32_t addr, uint32_t val)
{
    write16(addr, (val >> 16) & 0xFFFF);
    write16(addr + 2, val & 0xFFFF);
}

} // namespace cps1
