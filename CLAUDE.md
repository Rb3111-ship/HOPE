# CLAUDE.md: HOPE Lullaby Device

Context for Claude (and humans) picking this project up later. Last updated: **2026-10-03**.

## What this is
HOPE: a baby lullaby / night-light device. STM32F412RET6 (**WeAct core board**, 8 MHz HSE → 100 MHz) running FreeRTOS (CMSIS-RTOS v2). It has:
- an SH1107 128×128 I2C OLED
- a DFPlayer Mini (MP3 from SD)
- a Bluetooth audio module (PCB says HX-M18, README says MH-MX8)
- an HW-104 (PAM8403) amplifier and 2 speakers
- a 16-LED WS2812B ring
- a DS3231 RTC with an AT24C32 EEPROM
- a DHT22 temperature/humidity sensor
- 8 TTP223 touch sensors

The user (GitHub `Rb3111-ship`, repo `https://github.com/Rb3111-ship/HOPE`) is learning embedded development. Explain things in plain language, and give step-by-step physical instructions when hardware is involved.

## Source of truth
- **This folder (`C:\Users\whp27\Desktop\Hope_V1`) is the only live copy.**
- `D:\ST projects\Hope_V1` is an **old backup**. It was audited line by line and is fully merged here. Don't edit it, and don't treat it as newer. It's behind GitHub.
- Branch `main` tracks `origin/main`. A local branch `code-review-fixes` also exists; it's merged into main and can be deleted.
- The user builds and flashes from **STM32CubeIDE** (project imported from this folder). The ST-Link debug launch file still logs to `D:\` (harmless).

## Working agreements with the user
- **Work in phases** and report after each one. Say what was done, what's next, and what needs the user (hardware checks etc.).
- **Commit only when the user asks.** They have asked before; the last push was commit `da62a05`. Commits end with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- **Check before changing.** Example: the user asked for the SH1107 driver to be verified before it was modified.
- **Keep the firmware pin map fixed.** The user is redesigning the PCB around the existing pins (see `HARDWARE_GUIDE.md` §3).

## Key documents in this repo
| File | What |
|---|---|
| `CODE_REVIEW_SUMMARY.md` | Plain-English list of all issues found (C/H/M/L severity) + first-flash checklist |
| `CODE_REVIEW_FIXES.md` | Detailed fix reference + **progress log table** (every item C1–C4, H1–H6, M1–M14, L1–L11 with status) |
| `HARDWARE_GUIDE.md` | Full system description for the user's **single-board PCB redesign**: pin map, netlist, upgrades, layout/routing rules, connectors |
| `songs.md` | Song titles ↔ SD files (`MP3/0001.mp3`…`0025.mp3`; `0026.mp3` = alarm tone). Song #6 title still a placeholder |
| `README.md` | Project README (SD layout, architecture, directory tree) |

## Code map
| Path | Role |
|---|---|
| `Core/Src/main.c` | CubeMX init; creates queues, `i2c_dma_sem`, `i2c_mutex`, tasks (UI prio 2, music 3, light 1); `i2c1_bus_recover()`; the defaultTask deletes itself |
| `Core/Src/freertos.c` | Stack-overflow and malloc-failed hooks (DEBUG halts, release resets) |
| `Core/Src/Applications/ui_task.c` | UI state machine, touch EXTI callback + noise filter, alarm firing, sleep timer, light menu, volume |
| `Core/Src/Applications/music_task.c` | DFPlayer command loop: software track looping via `EVT_TRACK_FINISHED`, 60 ms command gap |
| `Core/Src/Applications/light_task.c` | WS2812 light modes, 20 ms frame |
| `Core/Src/UI/ui_renderer.c/.h` | All screens and overlays; **dino animation** (`draw_anim_scene`, text-row sprites); song list |
| `Core/Src/UI/ui_state.h` | UI states/events/overlays; `ui_msg_t` (now has a `tick` field) |
| `Core/Src/Services/*` | alarm (EEPROM persistence, midnight logic), time (`time_is_valid()`), music, light, sensor (keeps last good reading) |
| `Core/Src/Drivers/*` | DFPlayer (`df_*` API, play by name 0x12, RX parser), DHT22 (TIM4 capture, open-drain), DS3231, AT24C32 (0x57), WS2812 (TIM1 DMA), BLE power |
| `Drivers/OLED/ssd1306.c` + `ssd1306_conf.h` | OLED driver: **SH1107 init path** (`SSD1306_USE_SH1107`), I2C timeouts/recovery, `i2c_mutex`. Tunables: `SH1107_DISPLAY_OFFSET`, `SH1107_DCDC_SETTING`, `SSD1306_CONTRAST`, mirror flags |

### Firmware pin map (fixed, matches the PCB)
- PB6 SCL / PB7 SDA (OLED + RTC)
- PA9 TX → 1 k → DFPlayer RX; PA10 RX ← DFPlayer TX
- PA8 → 330 Ω → WS2812
- PB8 = DHT22; PB9 = BT power switch (high = on)
- Touch inputs:

| Pin | Function | Event |
|---|---|---|
| PB3 | UP | `EVT_BTN_NEXT` (list down/right) |
| PB4 | PLAY | |
| PB5 | VOL− | |
| PB12 | MENU | |
| PB13 | TIMER | |
| PB14 | VOL+ | |
| PB15 | DOWN | `EVT_BTN_PREV` (list up/left) |
| PD2 | LIGHT | |

- PB2 = spare output (`LED_TOGGLE`).

### Conventions / gotchas
- **CubeMX-generated files:** put user code inside `USER CODE BEGIN/END` blocks. Settings that must survive regeneration are mirrored in `Hope_V1.ioc`: the I2C1 EV/ER IRQs and the FreeRTOS heap (20480), FPU, stack check and malloc hook.
- **Line endings:** most sources are **CRLF**. Git-Bash `sed -i` silently converts files to LF; use the Edit tool or Python with `newline=''`. `ssd1306.c` and README were originally LF.
- **Non-ASCII:** `ui_renderer.c` contains UTF-8 box-drawing characters, so open it with `encoding='utf-8'` in Python.
- **Heredocs:** Python heredocs in the Bash tool mangle backslashes. Write scripts to files instead.
- The `.gitignore` (renamed from `.gitignore.txt`) ignores `Debug/`, `.settings/`, `*.launch`. `.settings/language.settings.xml` is still tracked, so `git add` of it warns.

## How to build-check without CubeIDE
The toolchain is CubeIDE's GCC 14.3:
```
TB=$(ls -d /d/STM32CubeIDE/STM32CubeIDE_2.1.1/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32*/tools/bin)
```
Compile every `.c` under:
- `Core/Src`
- `Drivers/OLED`
- `Drivers/STM32F4xx_HAL_Driver/Src`
- `Middlewares/Third_Party/FreeRTOS/Source` (excluding `portable/`, then add `portable/GCC/ARM_CM4F/port.c` and `portable/MemMang/heap_4.c`)

Compile flags:
```
-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb --specs=nano.specs -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F412Rx -O0 -ffunction-sections -fdata-sections -Wall
```
Include paths: `Core/Inc`, `Core/Src/{Applications,UI,Drivers,Services}`, `Drivers/OLED`, the HAL `Inc` and `Inc/Legacy` folders, CMSIS Device and Include, and FreeRTOS `include`, `CMSIS_RTOS_V2` and `portable/GCC/ARM_CM4F`.

Assemble `Core/Startup/startup_stm32f412retx.s` with `-x assembler-with-cpp`, then link with:
```
-T STM32F412RETX_FLASH.ld --specs=nosys.specs -Wl,--gc-sections -static -Wl,--start-group -lc -lm -Wl,--end-group
```
Last result: **0 errors, 0 warnings in user code** (also clean with `-Wextra`, apart from CubeMX's unused `argument`). Python libs aren't installed system-wide; PyMuPDF was installed to a temp folder to read the PCB PDFs.

## Status
### Done (all committed and pushed unless noted)
- **Code review:** all Critical/High/Medium/Low items fixed (see the progress log in `CODE_REVIEW_FIXES.md`). The SH1107 init path was added after verifying the old one was the stock SSD1306 sequence and was never called.
- **D: backup audit:** the Desktop copy is a complete superset.
- **Hardware bring-up with the user:**
  - The display didn't work because the **PCB swaps SDA/SCL at the display connector** (display "SDA" pad = PB6). The user crossed the wires and **the display now works**.
  - Pinout traced from `docs/assets/PCB_PCB2_*.pdf` (main board) and `PCB_PCB3_*.pdf` (power board): everything else matches the firmware. The DFPlayer TX/RX crossover is correct, despite the misleading "PA9" silkscreen label.

### Committed locally on 2026-10-03, NOT pushed yet, built OK, *not yet tested on hardware*
Commits `dfbb032`, `6e88cc8` and the docs commit. Run `git push` when the user asks.
1. **Volume works on every screen** (except Bluetooth mode). Previously it only worked on the music screens; the user thought the buttons had "stopped".
2. **Touch noise filter:** `touch_is_real()` re-reads the pin 25 ms after the edge, plus a 150 ms same-pin repeat guard. `ui_msg_t.tick` was added and the ISR fills `data.value` = pin and `tick`.
3. **Chrome-style running T-rex** replaces the bunny/stars/note animation (home + player screens). Auto-jumps over 3 cactus variants (jump params verified for ≥ 3 px clearance by simulation: `JUMP_H` 20, `JUMP_LEAD` 18, `JUMP_TAIL` 4), plus scrolling ground and clouds.
4. **`HARDWARE_GUIDE.md`** (new) and this `CLAUDE.md`.

Ask the user how the volume buttons, touch filter and dino behave on the real device.

### Open items / waiting on the user
- **Buzz** (hardware, ground noise; it varies with OLED content, touch-sensor LEDs and the BT radio). The user can't change the current boards, but can add wires. Suggested fixes:
  1. a separate audio GND wire from the DFPlayer GND to the HW-104 input GND, twisted with L/R
  2. a thick inter-board GND
  3. 470–1000 µF + 100 nF in parallel at the DFPlayer and display power pins
  4. optional: bypass the 2N2222 low-side BT switch
  - Software lever: lower `SSD1306_CONTRAST` (e.g. 0x40).
  - Also: the power board's 100 nF caps are **shunt to GND**, not series, which makes a ~1.6 kHz low-pass with the 1 k mixer (muffled audio).
- **PREV button (PB15) reportedly not working.** The code path is verified. Asked the user to test on the Menu screen and check the sensor LED / continuity to B15. No result yet.
- **Touch sensor mode unknown** (momentary vs toggle). If a key responds only every 2nd touch, it's in toggle mode.
- **The user is redesigning the PCB** as one merged (possibly round) board in EasyEDA using their existing footprints, following `HARDWARE_GUIDE.md`. The enclosure (3D print) can't change. They may come back for a review of the new schematic/PCB or Gerbers. Check it against `HARDWARE_GUIDE.md` §3 and §11.
- **Song #6 title** (`songs.md`, `ui_renderer.c` song_list index 5). When the user updates `songs.md`, sync it into `song_list[]` (`MAX_SONGS` in `ui_renderer.h` if the count changes).
- **Possible UX tweak offered:** swap UP/DOWN so UP moves the list up. Not requested yet.
- **Not done (needs hardware):** measuring task stack high-water marks (each task has 1000 words).
