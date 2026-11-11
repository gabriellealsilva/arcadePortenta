# STATUS.md — what is done, what is left

Last updated: 2026-11-11. Companion to [`AGENTS.md`](AGENTS.md) (what the project is, how it is laid out). Update this file whenever a task finishes or a new one is discovered.

Legend: `[x]` done · `[~]` partly done · `[ ]` not started

Current branch when written: `refactoring/separating_bootloader_menus`, with uncommitted work in `Firmware/Emulation/main.cpp`, `Videogame/CPS-1/UI/BootloaderMenu.h` and the new `Firmware/Emulation/SdlMenuController.h`.

---

## Recently done

- 2026-11-11 — Carrier PCB KiCad project moved into the repo at `Hardware/CarrierPCB/`; `CLAUDE.md` replaced by `AGENTS.md`; this file added.
- 2026-07-31 — Schematic fully regenerated (5 sheets, ERC 0 errors); `docs/HARDWARE_PLAN.md` written with the verified pin map; bootloader architecture decided (microSD → QSPI self-flash).
- Bootloader menu split into shared logic (`Videogame/CPS-1/UI/BootloaderMenu.h`) plus SDL renderer/controller for the host.
- ROM pipeline: `IRomSource` CRTP, `FilesystemRomSource`, `RomAssembler`, `RomLayout`/`GameLayouts` with 17 per-title layouts taken from MAME.
- Musashi 68000 wired into `Cps1System` with a CPS-1 memory map and VBlank interrupt.

---

## 1. Software — emulator core

- [x] Musashi 68k vendored and generated via CMake
- [x] Memory map: program ROM, work RAM, GFX RAM, palette RAM, CPS-A registers
- [x] 68k boots from ROM reset vector (verified with `1941`)
- [x] VBlank IRQ and ~one frame of cycles per `tick()`
- [~] Frame renderer: background colour, Scroll1 (8×8) and a basic 16×16 sprite pass only
- [ ] Scroll2 (16×16) and Scroll3 (32×32) layers
- [ ] Correct layer/sprite priority and the CPS-B layer-control registers
- [ ] CPS-B register emulation (ID/protection reads, raster IRQ)
- [ ] GFX ROM 4-bpp planar → linear decode with tile cache (currently not decoded at load)
- [ ] Palette-resolve to RGB565 per scanline (scanline-accurate rendering)
- [ ] Inputs: map ports `0x800000`–`0x80001F` (joysticks, buttons, coin, start, DIPs)
- [ ] Z80 sound CPU core + 68k→Z80 sound latch
- [ ] YM2151 (ymfm) and OKI MSM6295
- [ ] Audio mixer to 32 kHz S16 stereo
- [ ] EEPROM/QSound-free titles first; list titles that need extra hardware
- [ ] Save states (serialize 68k regs + RAM + CPS regs)
- [ ] Deterministic frame timing so a smoke test can hash the framebuffer

## 2. Software — host (SDL3)

- [x] Window, RGB565 framebuffer → texture upload
- [x] Bootloader menu on the 384×224 logical canvas; ESC returns to the menu
- [x] ROM path discovery (`argv[1]` or `ROMS_PATH`)
- [ ] Commit and merge the in-flight menu refactor (branch `refactoring/separating_bootloader_menus`)
- [ ] SDL audio queue backend
- [ ] Keyboard + gamepad → CPS-1 input ports
- [ ] CLI flags: game id, save-state load/save
- [ ] Replace placeholder ROM data: `megaman` is not a real dump, `sf2` GFX is missing/placeholder

## 3. Software — Portenta firmware

- [x] CubeMX project generated; CM7 and CM4 ELFs build with the cross-compile preset
- [x] VS Code "Build + Flash" task (STM32_Programmer_CLI over SWD)
- [~] `Firmware/Loop/mainFirmware.cpp` — entry stubs only (empty loops)
- [ ] Update `CPS_1.ioc` to the verified pin map (`docs/HARDWARE_PLAN.md` §7, checklist in §8) and regenerate
- [ ] OpenOCD config + cortex-debug launch profile; "blink an LED on M7" milestone
- [ ] Clock tree: M7 480 MHz, M4 240 MHz, PLL3 for the pixel clock (25.175 MHz and ~8 MHz)
- [ ] MPU and cache setup (framebuffer write-through, DMA buffers non-cacheable)
- [ ] FMC SDRAM init + framebuffer placement
- [ ] QSPI memory-mapped (XIP) init at `0x90000000`
- [ ] SDMMC2 + FAT32 reader → `SdRomSource` (same `IRomSource` interface)
- [ ] Self-flashing bootloader: interleave in SDRAM → erase/program QSPI → header block (game id + CRC) → boot XIP
- [ ] `QspiRomSource` for the running emulator
- [ ] LTDC init (384×224, 31 kHz / 15.7 kHz selected by jumper) + DMA2D
- [ ] SAI/I2S2 double-buffered DMA to the PCM5102A
- [ ] 74HC165 bit-banged input driver (3 GPIOs) behind the common input struct
- [ ] M7/M4 split: M7 = 68k + video, M4 = Z80 + sound chips; HSEM + SPSC ring in SRAM4
- [ ] UART1 debug console (SWO is not available), watchdog, brownout config

## 4. Repository, tooling, CI

- [x] Layered CMake build with host and cross-compile presets
- [x] `.clang-format`, `-Werror` warning set
- [ ] Smoke test: boot `1941`/`sf2ce` to frame 600 and hash the framebuffer
- [ ] GitHub Actions: host build + smoke test
- [ ] Decide whether `docs/` should stay git-ignored (it holds the hardware plan; agents cloning the repo will not see it)
- [ ] Refresh `docs/IMPLEMENTATION_PLAN.md`: it still shows the dropped THS8136 / 74LVC245 / PC-flasher designs
- [ ] README: toolchain bootstrap section
- [ ] Stretch: permissive-licence 68k core, 15 kHz CRT polish, in-game menu, second platform

## 5. Hardware — carrier PCB (`Hardware/CarrierPCB/`)

Schematic
- [x] Root sheet + `portenta` (two DF40 connectors, microSD, SWD/UART headers, mode jumper/LED)
- [x] `vga` (R-2R ladder DAC, 74HCT244 sync buffer, 74HC86 CSYNC, DE-15, JAMMA video takeoff, test points)
- [x] `audio` (PCM5102A application circuit, bead + net-tie, line-out jack, DNP amp footprint area)
- [x] `power` (5 V entry/protection, DNP 12 V π-filter, chassis hybrid ground)
- [x] `inputs` (JAMMA 56-pin edge, 4× 74HC165 chain, kick harness + AUX headers, DNP coin-counter driver)
- [x] ERC: 0 errors; cross-sheet netlist verified
- [ ] Hand-review of the generated sheets (once editing by hand starts, stop using `gen_sch.py`)
- [ ] Assign footprints for every part, run a final ERC, export BOM

Verify before layout (from `docs/HARDWARE_PLAN.md` §12)
- [ ] DF40 stacking height vs Portenta bottom-side clearance
- [ ] DF40 footprint pad numbering matches the odd/even schematic symbol (old symbol Value said "Counter_Clockwise")
- [ ] V-SDCARD (J1-74) sources 3V3 on the Portenta (fallback JP100 exists)
- [ ] PLL3 can hit 25.175 MHz and 8 MHz from the 25 MHz HSE with acceptable jitter
- [ ] Portenta 3V3 output has headroom for PCM5102 + logic (else add an LDO)
- [ ] Dual-bonded pads (PB3, PA0, PA4, PC2, PC3, PH11, PH12, PI5–PI7) keep their twin pad NC
- [ ] CRT chassis input levels once a monitor is available

Layout (`ArcadePortenta.kicad_pcb` is still empty of design work)
- [ ] Stackup (4-layer: SIG / GND / PWR / SIG), board outline, mounting holes
- [ ] Place Portenta module and DF40s, then connectors, then the rest (order in `HARDWARE_PLAN.md` §9)
- [ ] Route per the rules in `HARDWARE_PLAN.md` §9 (VGA RGB matching, pixel-clock stitching, I²S away from switchers)
- [ ] DRC clean, 3D check, gerbers

Fab and bring-up
- [ ] Stage 0: FabLab (RWTH) prototype without a custom carrier
- [ ] Stage 1: order boards (×2) via AISLER, parts via the shopping list in `HARDWARE_PLAN.md` §10
- [ ] Power-only smoke test with no module seated
- [ ] Seat the Portenta, check SWD in OpenOCD
- [ ] Colour bars over VGA, 1 kHz sine on line out, every JAMMA input arrives at its GPIO

## 6. Integration milestones

- [ ] SF2 (or `sf2ce`) attract mode at 60 fps with audio on the host
- [ ] Same ROM booted from QSPI XIP on real hardware
- [ ] Frame timing and input latency measured
- [ ] 30-minute thermal soak; basic EMI sanity check
