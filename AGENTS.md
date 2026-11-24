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
| `Videogame/CPS-1/core/` | Platform-independent CPS-1 emulation: `Cps1System` (memory map, Musashi glue, frame render), `RomLayout.h` + `GameLayouts.cpp` + `Games/*.h` (per-title ROM chip layouts) |
| `Videogame/CPS-1/Rom/` | `RomDefs.h` (`RomEntry`, `GameDescriptor`) and `IRomSource.h` (CRTP ROM-source base) |
| `Videogame/CPS-1/UI/BootloaderMenu.h` | Backend-neutral bootloader menu logic, shared by emulator and hardware |
| `Firmware/Emulation/` | **Host (Windows/SDL3) backend**: `main.cpp`, `FilesystemRomSource`, `RomAssembler`, `SdlMenuRenderer.h`, `SdlMenuController.h`, `Log.h` |
| `Firmware/Loop/` | Firmware entry points for CM7 / CM4 (`mainFirmware.cpp`, currently empty loops) |
| `Hardware/Portenta/CPS1/` | STM32CubeMX project (`CPS_1.ioc`, `CPS_1.cubemx_script`), generated HAL init for CM7/CM4/Common, linker scripts in `Memory/` |
| `Hardware/CarrierPCB/` | **KiCad 10 project for the carrier PCB** (schematics, empty PCB layout, `gen_sch.py` generator) |
| `HAL/ST/` | Vendored STM32H7 HAL + CMSIS (git-ignored) |
| `External/musashi/` | CMake wrapper around the Musashi 68000 core (fetched/generated at configure time) |
| `cmake/` | Build modules: toolchain, flags, HAL, CubeMX, firmware, console targets |
| `ROMS/cps1/<game>/` | Local ROM chip dumps (git-ignored, user-supplied): `1941`, `sf2`, `sf2ce`, `megaman` |
| `docs/` | `HARDWARE_PLAN.md` (authoritative hardware decisions + pin map), `IMPLEMENTATION_PLAN.md` (original roadmap, partly superseded). **Git-ignored — local only.** |
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
| Internal flash | `0x08000000` | 2 MB | Firmware |
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

**Firmware (ARM cross-compile)**

```
cmake --preset=win-cross-compile-debug
cmake --build --preset=win-cross-compile-debug
cmake --build --preset=win-cross-compile-debug --target smoke_builds   # build every target
```

Outputs: `build/win-cross-compile-debug/Videogame/Portenta_CM7_CPS1_Debug.elf` and `…_CM4_CPS1_Debug.elf`.

**Flash:** VS Code task "Build + Flash", or `STM32_Programmer_CLI --connect port=swd --download <elf> -hardRst --start`.

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

- `ROMS/cps1/megaman/` is **not a real dump** (garbage 68k reset vector). `sf2` has missing or placeholder GFX ROMs. Use **`1941`** or **`sf2ce`** for boot and regression checks.
- ROM files are copyrighted and git-ignored. Never commit them; never add a feature that downloads them.
- Musashi is under a non-commercial licence. Swapping it for a permissive 68k core is a prerequisite if the project is ever distributed.
- `.gitignore` ignores `*.json`, `/docs/`, `/ROMS/` and `/HAL/ST` on purpose. If a new tracked `.json` file is needed, add a `!` exception.
- The H7 cache/MPU setup is the usual source of strange bugs: framebuffers write-through, DMA buffers non-cacheable.

## 8. Working agreements for agents

1. Read `STATUS.md` first. When you finish a task, tick it there and add a one-line note under "Recently done". Add newly discovered work under "To do".
2. Keep the layering rule (section 3). If a change needs a platform include inside `Videogame/`, add an interface instead.
3. Do not regenerate CubeMX output, re-run `gen_sch.py`, or edit generated files without being asked.
4. Do not commit, push or open pull requests unless asked.
5. Prefer verifying with a real build over reasoning about it. Report build failures honestly.
6. Keep this file accurate. If you change the structure, build commands or conventions, update it in the same change.
