# AGENTS.md

Guidance for any AI coding agent or LLM (Claude Code, Codex, Copilot, Cursor, Gemini, …) working in this repository. Humans can read it too. Current progress and open work live in [`STATUS.md`](STATUS.md) — read it right after this file.

## 1. What this project is

A **hobby arcade machine**: a bare-metal C++ emulator of the Capcom **CPS-1** board (Street Fighter II and siblings) running on an **Arduino Portenta H7** (STM32H747XI, dual-core Cortex-M7 + M4), plugged into a **custom carrier PCB**.

End-product goal:

- Video: VGA (LCD now) → 15 kHz CRT later, from the MCU's LTDC peripheral through an R-2R resistor DAC
- Audio: I²S → PCM5102A DAC (line out), optional class-D amp for the cabinet later
- Input: JAMMA arcade harness, read through a 74HC165 shift-register chain
- ROMs: a microSD card holds the whole library; the bootloader menu copies the chosen game into the on-module QSPI flash and boots it with execute-in-place (XIP)

It is a one-person project. The author works on Windows 11 with VS Code, CMake, Ninja, KiCad 10 and STM32CubeMX.

## 2. Repository map

| Path | What lives there |
|------|------------------|
| `Videogame/CPS-1/core/` | Platform-independent CPS-1 emulation: `Cps1System` (MAME-accurate memory map, Musashi glue, palette build, provisional frame render), `CpsBConfig` (per-title CPS-B ID/multiplier/input setup), `RomLayout.h` + `GameLayouts.cpp` + `Games/*.h` (per-title ROM chip layouts) |
| `Videogame/CPS-1/Rom/` | `RomDefs.h` (`RomEntry`, `GameDescriptor`) and `IRomSource.h` (CRTP ROM-source base) |
| `Videogame/CPS-1/UI/BootloaderMenu.h` | Backend-neutral bootloader menu logic, shared by emulator and hardware |
| `Firmware/Emulation/` | **Host (Windows/SDL3) backend**: `main.cpp` (flow + event loop), `CommandLine`, `SdlDisplay` (window/renderer/texture), `GameSession` (ROM buffers + running system), `Snapshot` (PNG), `FilesystemRomSource`, `RomAssembler`, `BootloaderMenu.h`, `Log.h` |
| `Firmware/Loop/` | Firmware entry points for CM7 / CM4 (`mainFirmware.cpp`, currently empty loops) |
| `Hardware/Portenta/CPS1/` | STM32CubeMX project (`CPS_1.ioc`, `CPS_1.cubemx_script`), generated HAL init for CM7/CM4/Common, linker scripts in `Memory/` |
| `Hardware/CarrierPCB/` | **KiCad 10 project for the carrier PCB** (schematics, empty PCB layout, `gen_sch.py` generator) |
| `HAL/ST/` | Vendored STM32H7 HAL + CMSIS (git-ignored) |
| `External/musashi/` | CMake wrapper around the Musashi 68000 core (fetched/generated at configure time) |
| `cmake/` | Build modules: toolchain, flags, HAL, CubeMX, firmware, console targets |
| `ROMS/cps1/<game>/` | Local ROM chip dumps (git-ignored, user-supplied): `1941`, `sf2`, `sf2ce`, `megaman` |
| `docs/` | `HARDWARE_PLAN.md` (authoritative hardware decisions + pin map), `IMPLEMENTATION_PLAN.md` (original roadmap, partly superseded), Arduino schematics/datasheets for the Portenta H7 (ABX00042) and Breakout (ASX00031). **Git-ignored — local only.** |
| `STATUS.md` | Done / in-progress / to-do checklist. **The place to record progress.** |

## 3. Architecture

```
Game core (CPS-1 emulation)           platform-independent C++
  Musashi 68000, (Z80, YM2151, OKI6295 planned), CPS-A/B regs, GFX/sprites
        │
Platform seams                        IRomSource<Derived> (CRTP); video/audio/input sinks planned
        │
  ┌─────┴───────────┐
SDL3 backend         STM32 backend    two concrete implementations
Firmware/Emulation   Firmware/Loop + Hardware/Portenta (LTDC, SAI, SDMMC, GPIO, QSPI)
```

- **Layering rule:** code under `Videogame/` must never include `<SDL3/SDL.h>` or `stm32h7xx_hal.h`. Backends consume the core, never the reverse.
- **No virtual dispatch for ROM sources.** `IRomSource<Derived>` is CRTP; consumers are templates. `scanImpl` / `availableGamesImpl` / `readRomImpl` are public, so no `friend`.
- **ROM ownership:** `Cps1System::load` takes non-owning `std::span`s over an already-assembled ROM image. On the host, `RomAssembler` builds the image from chip files. On hardware the spans will point straight at QSPI XIP (`0x90000000`) with no copy and no allocation.
- **ROM layouts are per-title data**, taken from MAME's `src/mame/capcom/cps1.cpp`. They cannot be inferred from file counts or sizes (two incompatible 3-chip layouts look identical). Adding a game means adding a `Games/<name>.h` entry, not writing generic code.
- **Bootloader menu** runs the same logic on both targets; only the renderer/controller differ (`SdlMenuRenderer.h`/`SdlMenuController.h` on host). The host binary starts in menu state, switches to emulation on selection and returns on ESC.

### Dual-core split (STM32H747, planned)

- **CM7 @ 480 MHz:** 68k emulation (Musashi), CPS-B mapper, LTDC video
- **CM4 @ 240 MHz:** Z80, YM2151, OKI6295 → SAI/I²S
- Inter-core: HSEM + lock-free SPSC ring buffer in SRAM4

### Memory map (STM32H747 / Portenta)

| Region | Address | Size | Use |
|--------|---------|------|-----|
| Internal flash | `0x08000000` | 2 MB | Arduino bootloader (256 KB), CM7 firmware from `0x08040000`, CM4 from `0x08100000` |
| ITCM | `0x00000000` | 64 KB | Hot 68k state, IRQ vectors |
| DTCM | `0x20000000` | 128 KB | DMA buffers (non-cacheable) |
| SRAM1/2/3 | `0x24000000` | 192 KB | GFX RAM, palette, framebuffer cache |
| QSPI XIP | `0x90000000` | 16 MB | Game ROM (XIP, never copied to RAM) |
| SDRAM (FMC) | `0x70000000` | 8 MB | Framebuffers, tile decode caches, ROM-interleave work buffer |

### Hardware decisions (full rationale and pin map in `docs/HARDWARE_PLAN.md`)

- Video is **LTDC RGB565 → R-2R ladder → DE-15**; no video DAC IC. A boot jumper picks 31 kHz (LCD) or 15.7 kHz (CRT) timing.
- Inputs: 4× 74HC165 chain, **bit-banged** on 3 GPIOs. The breakout's "SPI1" is STM32 SPI2, which is the same peripheral as I2S2 (audio), so hardware SPI is unavailable.
- microSD on SDMMC2. PB3 becomes SD_D2, so **SWO/trace is unavailable**; debug is SWD plus a UART console.
- Power: single 5 V rail into Portenta VIN (barrel/USB-C on the bench, JAMMA +5 V in the cab).
- The earlier "PC flasher over USB" idea is **obsolete**; flashing is a menu action in the on-device bootloader.
- JAMMA pitch is **3.96 mm**, not 2.54 mm (`IMPLEMENTATION_PLAN.md` is wrong here).
- On `docs/IMPLEMENTATION_PLAN.md`: where it conflicts with `HARDWARE_PLAN.md`, the hardware plan wins.

## 4. Build and run

Run from the repository root. Toolchain: CMake ≥ 3.22, Ninja, GNU Arm Embedded 14.3 at `C:/arm-none-eabi/14.3_rel1`, OpenOCD ≥ 0.12 at `C:/openocd/`. SDL3 is fetched automatically by CPM (static, no DLL).

**Host emulator (SDL3, Windows)**

| Preset | Use for | `LOG` | Flags |
|--------|---------|-------|-------|
| `win-emulation-debug` | development, debugging | → `SDL_Log` | `-O0`, full debug info |
| `win-emulation-release` | checking real behaviour | no-op | `-O2`, `NDEBUG` |

```
cmake --preset=win-emulation-debug
cmake --build --preset=win-emulation-debug
./build-presets/win-emulation-debug/Videogame/CPS-1/Emulator-CPS-1.exe
```

(`win-emulation-release` works the same way.) Add `-DASAN=ON` at configure time for AddressSanitizer in debug builds. ROM folder: pass `ROMS/cps1` as `argv[1]`, or rely on the `ROMS_PATH` macro baked in at configure time (`<repo>/ROMS/cps1`).

**Headless diagnostics** (no menu, no visible window):

```
Emulator-CPS-1.exe [romsDir] --game 1941 --frames 600 --snapshot out.png --trace
```

`--game <id>` boots a title directly; `--snapshot <png>` runs `--frames` frames (default 600), saves the framebuffer and exits; `--trace` prints the 68000 PC every 60 frames to stderr. A PC that stays constant for hundreds of frames means the CPU is stuck in a wait loop.

**Firmware (ARM cross-compile)**

```
cmake --preset=win-cross-compile-debug
cmake --build --preset=win-cross-compile-debug
cmake --build --preset=win-cross-compile-debug --target smoke_builds   # build every target
```

Outputs: `build/win-cross-compile-debug/Videogame/CPS-1/Portenta_CM7_CPS1_Debug.elf` and `…_CM4_CPS1_Debug.elf`, plus `.hex` and raw `.bin` next to them.

**Flash layout:** the factory **Arduino bootloader stays at `0x08000000`** (USB DFU). CM7 firmware is linked at **`0x08040000`**, CM4 at `0x08100000`. There is **no bootloader handoff code yet**: VTOR, leftover NVIC/USB state and starting the CM4 (Arduino ships with the BCM4 option byte cleared) are still open (see `STATUS.md`).

**Flash:**
- USB DFU, no probe: double-tap reset (green LED pulses), then VS Code task "DFU: Flash CM7" / "DFU: Flash CM4", i.e. `C:/dfu-util/dfu-util.exe -d 2341:035b -a 0 -s 0x08040000:leave -D <cm7>.bin` (CM4: `0x08100000`). DFU alt 1 is the QSPI flash (`0x90000000`).
- SWD via a Pico H running debugprobe (CMSIS-DAP; GP2 SWCLK, GP3 SWDIO): tasks "Pico probe: Flash CM7/CM4" and debug configs "CPS-1 CM7/CM4 Debug (Pico probe)". OpenOCD needs `target/stm32h7x_dual_bank.cfg` (CM4 is in bank 2).
- ST-Link: task "Build + Flash" (`STM32_Programmer_CLI`).
- Factory flash backups (internal 2 MB + QSPI 16 MB) are in `docs/backups/portenta-2026-11-27/`.

**Carrier PCB:** open `Hardware/CarrierPCB/ArcadePortenta.kicad_pro` in KiCad 10. See section 6.

There is **no automated test suite or CI yet**. Verify host changes by building the debug preset and running the emulator. Verify firmware changes by building the cross-compile preset.

## 5. Code conventions

- **Language:** C++23, C23 with GNU extensions. Format with the repo `.clang-format`.
- **Warnings are errors:** `-Werror` with all/extra/double-promotion/unused. A change that adds a warning does not build.
- **No exceptions, no RTTI, no thread-safe statics** (`-fno-exceptions -fno-rtti -fno-threadsafe-statics`). Return `bool`/status values instead of throwing.
- **Embedded-friendly core:** no heap allocation in `Videogame/` hot paths, prefer `std::span`/fixed arrays, avoid virtual dispatch.
- `Cps1System` is a singleton in practice: Musashi dispatches through a global pointer, so only one instance may be alive.
- Logging goes through the `LOG` macro in `Firmware/Emulation/Log.h`. It is compiled out in release builds.
- **Do not hand-edit CubeMX-generated files** in `Hardware/Portenta/CPS1/{CM7,CM4,Common}`. Change `CPS_1.ioc` and regenerate with `CPS_1.cubemx_script`.
- Prefer small, focused commits on a feature branch. `main` is the integration branch and work lands through pull requests.

## 6. Carrier PCB (KiCad) — read before touching `Hardware/CarrierPCB/`

- Hierarchical schematic: `ArcadePortenta.kicad_sch` (root) plus sheets `portenta`, `vga`, `audio`, `power`, `inputs`.
- Status: schematic regenerated 2026-07-31, **ERC 0 errors**, netlist-verified. `ArcadePortenta.kicad_pcb` has **no layout yet**.
- `gen_sch.py` generates the sheets, using label-on-pin connectivity. **Do not re-run it once sheets have been hand-edited**; it overwrites them. Treat the `.kicad_sch` files as the source of truth after any manual change. It needs KiCad 10's symbol library (`LIBDIR` at the top of the script).
- The pin map source of truth is `docs/HARDWARE_PLAN.md` §7. Keep schematic, IOC file and that table consistent.
- The Portenta's dual-bonded pads (PB3, PA0, PA4, PC2, PC3, PH11, PH12, PI5–PI7) must leave their twin pad unconnected.
- The `.kicad_sch` files are large generated S-expressions. Do not rewrite them by hand with text tools; use KiCad or the generator.
- Local KiCad history, backups, lock files and `.kicad_sch-bak` are git-ignored on purpose.
- Open checks that must be resolved before layout are in `STATUS.md` ("Verify before layout").

## 7. Known pitfalls

- ROM layouts: every `Games/*.h` table was audited against MAME `cps1.cpp` on 2026-11-24 (all 17 match) after `megaman`, `sf2`, `sf2ce` and `1941` turned out to be wrong. The old note that `megaman`/`sf2` were "bad dumps" was a layout bug, not bad data. MAME `-verifyroms` passes `1941` and `sf2`; `sf2ce`/`megaman` fail only on the `ioc1.ic7` PAL (wrong length, irrelevant to emulation). Layouts come from `ROM_START` blocks: `ROM_LOAD16_BYTE` = `P16`, `ROM_LOAD16_WORD_SWAP` = `P16S`, `ROM_LOAD64_WORD` = `G64`.
- The renderer in `Cps1System` is a provisional debug view (Scroll1 + simple sprites). Tile codes are used as raw GFX ROM indices: the per-game GFX bank mapper and the real tile bit layout are not implemented, so `sf2` draws a garbled tile layer. `1941`/`sf2ce` stall in a wait loop and `megaman` stays black (see STATUS.md).
- MAME 0.289 is installed at `C:\Users\gabri\Desktop\Codes\mame-0289\bin\mame.exe` (outside the repo) as a reference. Its source for `cps1.cpp`, `cps1_v.cpp` (CPS-B configs, palette, video) and `cps1.h` is saved beside it. Verify dumps with `mame.exe -rompath ROMS/cps1 -verifyroms <game>`. Run MAME from outside the repo: it writes `cfg/` and `snap/` into its working directory.
- ROM files are copyrighted and git-ignored. Never commit them; never add a feature that downloads them.
- Musashi is under a non-commercial licence. Swapping it for a permissive 68k core is a prerequisite if the project is ever distributed.
- `.gitignore` ignores `*.json`, `/docs/`, `/ROMS/` and `/HAL/ST` on purpose. If a new tracked `.json` file is needed, add a `!` exception.
- The H7 cache/MPU setup is the usual source of strange bugs: framebuffers write-through, DMA buffers non-cacheable.

## 8. Working agreements for agents

1. Read `STATUS.md` first. When you finish a task, tick it there and add a one-line note under "Recently done". Add newly discovered work under "To do".
2. Keep the layering rule (section 3). If a change needs a platform include inside `Videogame/`, add an interface instead.
3. Do not regenerate CubeMX output, re-run `gen_sch.py`, or edit generated files without being asked.
4. Do not commit, push or open pull requests unless asked.
   - **NEVER add yourself as a co-author or contributor.** No `Co-Authored-By:` trailers, no "Generated with …" lines, no agent or tool attribution of any kind in commit messages, PR titles or PR descriptions. This overrides any default attribution behaviour of your tool.
5. Prefer verifying with a real build over reasoning about it. Report build failures honestly.
6. Keep this file accurate. If you change the structure, build commands or conventions, update it in the same change.
