# STATUS.md — what is done, what is left

Last updated: 2026-11-24. Companion to [`AGENTS.md`](AGENTS.md) (what the project is, how it is laid out). Update this file whenever a task finishes or a new one is discovered.

Legend: `[x]` done · `[~]` partly done · `[ ]` not started

---

## Recently done
- 2026-11-24: MAME 0.289 installed as reference. Every `Games/*.h` table audited against MAME `cps1.cpp` (all 17 match); fixed `megaman`, `sf2`, `sf2ce`, `1941`. Real CPS-1 memory map + CPS-B config + palette build in `Cps1System`; headless snapshot CLI; `main.cpp` split into `CommandLine`/`SdlDisplay`/`GameSession`.

- 2026-11-27 — Pico H flashed with debugprobe v2.3.1 (CMSIS-DAP SWD probe: GP2 SWCLK, GP3 SWDIO); "Pico probe" CM7/CM4 flash tasks + debug configs added to the local `.vscode/`; factory Portenta internal flash (2 MB) and QSPI (16 MB) backed up over DFU to `docs/backups/portenta-2026-11-27/`. SWD wiring to the Breakout not yet tested. CM7 now linked at `0x08040000` behind the Arduino bootloader, flashable over USB DFU; handoff code still to do, not yet run on hardware.
- 2026-11-24 — Portenta H7 (SL-ABX00042) and Breakout (ASX00031) schematics reviewed (PDFs in `docs/`): every carrier DF40 pin checked, one error found (SD power pin), module facts and the Stage 0 Breakout plan added to `docs/HARDWARE_PLAN.md`.
- 2026-11-11 — Carrier PCB KiCad project moved into the repo at `Hardware/CarrierPCB/`; `CLAUDE.md` replaced by `AGENTS.md`; this file added.
- 2026-07-31 — Schematic fully regenerated (5 sheets, ERC 0 errors); `docs/HARDWARE_PLAN.md` written with the verified pin map; bootloader architecture decided (microSD → QSPI self-flash).
- Bootloader menu split into shared logic (`Videogame/CPS-1/UI/BootloaderMenu.h`) plus SDL renderer/controller for the host.
- ROM pipeline: `IRomSource` CRTP, `FilesystemRomSource`, `RomAssembler`, `RomLayout`/`GameLayouts` with 17 per-title layouts taken from MAME.
- Musashi 68000 wired into `Cps1System` with a CPS-1 memory map and VBlank interrupt.

---

## 1. Software — emulator core

- [x] Musashi 68k vendored and generated via CMake
- [x] Memory map follows MAME `main_map`: I/O `0x800000`, CPS-A `0x800100`, CPS-B `0x800140`, GFX RAM `0x900000`, work RAM `0xFF0000` (2026-11-24; was wrong before)
- [x] 68k boots from ROM reset vector (verified with `1941`)
- [x] VBlank IRQ and ~one frame of cycles per `tick()`
- [~] Frame renderer: background colour, Scroll1 (8×8) and a basic 16×16 sprite pass only
- [ ] Scroll2 (16×16) and Scroll3 (32×32) layers
- [ ] Correct layer/sprite priority and the CPS-B layer-control registers
- [~] CPS-B registers: per-title ID and multiplier protection done for all 17 titles (`core/CpsBConfig.*`); raster IRQ and layer-control use still to do
- [ ] GFX ROM 4-bpp planar → linear decode with tile cache (currently not decoded at load)
- [ ] Per-game GFX bank mapper (MAME `gfx_range` tables) — tile codes are used as raw ROM indices today
- [ ] Palette-resolve to RGB565 per scanline (scanline-accurate rendering)
- [~] Inputs: ports `0x800000`–`0x80001F` return idle (all ones); real joystick/coin/start/DIP values still to do
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
- [x] CLI flags: `--game`, `--frames`, `--snapshot <png>`, `--trace` (headless PNG snapshot + PC trace)
- [ ] CLI flags: save-state load/save
- [ ] Investigate why `1941` and `sf2ce` sit in a wait loop (PC 0x000840 / 0x0001C8) and `megaman` stays black; `sf2` draws a garbled tile layer

## 3. Software — Portenta firmware

- [x] CubeMX project generated; CM7 and CM4 ELFs build with the cross-compile preset
- [x] VS Code "Build + Flash" task (STM32_Programmer_CLI over SWD)
- [~] `Firmware/Loop/mainFirmware.cpp` — entry stubs only (empty loops)
- [ ] Update `CPS_1.ioc` to the verified pin map (`docs/HARDWARE_PLAN.md` §7, checklist in §8) and regenerate
- [x] OpenOCD config + cortex-debug launch profile (Pico H debugprobe, CMSIS-DAP)
- [x] Keep the Arduino bootloader: CM7 linked at `0x08040000`, `.bin` post-build, VS Code "DFU: Flash CM7/CM4" tasks
- [ ] Bootloader handoff in `CM7/Src/main.c` USER CODE: set VTOR, clear leftover NVIC/SysTick/USB state, start the CM4 when BCM4 is cleared (else the D2 boot handshake times out into `Error_Handler`); check `HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY)` vs the bootloader's setting (write-once, mismatch silently ignored)
- [ ] `MX_SDMMC2_SD_Init()` calls `Error_Handler()` when no card answers — firmware hangs at boot without an SD card wired
- [ ] Stage 0 bench test (no VGA yet): microSD on the Breakout → read one ROM → assemble → program QSPI → verify, run from `main`; then move it into the bootloader menu
- [ ] "Blink an LED on M7" milestone
- [ ] Clock tree: M7 480 MHz, M4 240 MHz, PLL3 for the pixel clock (25.175 MHz and ~8 MHz). HSE and LSE are oscillators (bypass mode); drive PH1 (OSCEN) high before starting HSE
- [ ] Open the PA0/PC2/PC3 analog switches at boot (`HAL_SYSCFG_AnalogSwitchConfig`): they are 165-chain digital pins
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
- [ ] **Fix in KiCad:** SD socket power label VSD is on J1-74 (SPDIF_TX, NC on the module); move it to J1-72 (VCC 3V3). PE2/PB2 NC labels belong on J1-66/68. Update the `gen_sch.py` table to match (do not re-run it)
- [x] DF40 stacking height: DF40HC(3.5)-80DS-0.4V(51), same part as the Breakout
- [ ] DF40 footprint pad numbering matches the odd/even schematic symbol (old symbol Value said "Counter_Clockwise")
- [x] V-SDCARD: no such pin on the H7; the SD socket runs from VCC 3V3 (see the fix above)
- [ ] PLL3 can hit 25.175 MHz and 8 MHz from the 25 MHz HSE with acceptable jitter
- [x] Portenta 3V3 output: 1 A max, no extra LDO needed
- [~] Dual-bonded pads (PB3, PA0, PA4, PC2, PC3, PH11, PH12, PI5–PI7) keep their twin pad NC: correct in the `gen_sch.py` table against SL-ABX00042; re-check in KiCad during hand review
- [ ] PJ6 (LTDC_R7) also drives the on-module USB3320 PHY ID pin: scope R7 on the Breakout with the PHY in reset (PJ4 low)
- [ ] CRT chassis input levels once a monitor is available

Layout (`ArcadePortenta.kicad_pcb` is still empty of design work)
- [ ] Stackup (4-layer: SIG / GND / PWR / SIG), board outline, mounting holes
- [ ] Place Portenta module and DF40s, then connectors, then the rest (order in `HARDWARE_PLAN.md` §9)
- [ ] Route per the rules in `HARDWARE_PLAN.md` §9 (VGA RGB matching, pixel-clock stitching, I²S away from switchers)
- [ ] DRC clean, 3D check, gerbers

Fab and bring-up
- [~] Stage 0 on the Portenta Breakout (ASX00031, bought 2026-11): plan and wiring map in `docs/HARDWARE_PLAN.md` §9
  - [ ] Solder female 2.54 mm headers; leave jumpers J9–J13 open, SW1 off
  - [ ] Blink the on-module RGB LED (PK5–PK7), UART1 console via USB-UART
  - [ ] SD/FAT32 + QSPI self-flash (no extra hardware); jumper SD CD → GPIO_0 (PC13)
  - [ ] PCM5102A module on the I2S header: 1 kHz sine
  - [ ] Perfboard 5/6/5 ladder (carrier values) → DE-15: colour bars
  - [ ] One 74HC165 + buttons, then the full chain
- [ ] Stage 0b (optional): FabLab (RWTH) milled shield to replace the perfboard
- [ ] Stage 1: order boards (×2) via AISLER, parts via the shopping list in `HARDWARE_PLAN.md` §10
- [ ] Power-only smoke test with no module seated
- [ ] Seat the Portenta, check SWD in OpenOCD
- [ ] Colour bars over VGA, 1 kHz sine on line out, every JAMMA input arrives at its GPIO

## 6. Integration milestones

- [ ] SF2 (or `sf2ce`) attract mode at 60 fps with audio on the host
- [ ] Same ROM booted from QSPI XIP on real hardware
- [ ] Frame timing and input latency measured
- [ ] 30-minute thermal soak; basic EMI sanity check
