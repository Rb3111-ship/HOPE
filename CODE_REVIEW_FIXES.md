# HOPE Firmware — Detailed Fix List (implementation reference)

> Audience: Claude (or a developer) implementing the fixes later.
> Review date: 2026-10-03. Line numbers refer to the working tree at review time (uncommitted changes on `main`, last commit `649e89e`).
> No code has been changed yet. This document only describes what to change.

## Progress log (update as phases complete)

**Source of truth = this Desktop folder.** `D:\ST projects\Hope_V1` (commit `f4c46c8`, 27 May) was merged in on 2026-10-03 and is now only a backup. Don't edit it.

Phase plan agreed with the user: Phase 1 = Critical, Phase 2 = High, Phase 3 = Medium, Phase 4 = Low, **last phase = the SH1107 screen-driver check (C3)**. The user believes the driver may already be adapted for the SH1107, so check before changing anything.

| Item | Status | Notes |
|---|---|---|
| C1 | ✅ Done (phase 1) | `ssd1306_Init()` at the top of `ui_Task`; `HAL_Delay` → `vTaskDelay` in ssd1306.c |
| C2 | ✅ Done (phase 1) | I2C1 EV/ER IRQs (prio 5) in `.ioc`, `stm32f4xx_hal_msp.c`, `stm32f4xx_it.c/.h` |
| C3 | ✅ Done (last phase) | Checked first. `ssd1306_Init()` was never called in any commit, and the 128-row branch (`0xFF`, "Luma SH1106") is the unmodified afiskon upstream code, so it was never adapted for the SH1107. On an SH1107 it only half-works by accident: `0x20` = page mode, the multiplex stays at the 128 POR value, and `0x40/0xDA/0x8D` are invalid, so the DC-DC (`0xAD`) is never set. Added a `SSD1306_USE_SH1107` init path based on Adafruit's SH1107 128x128 sequence: AE, D5 51, 20, 81 c, AD 8A, A1/A0, C8/C0, DC 00, D3 off, D9 22, DB 35, A8 7F, A4, A6, clear RAM, 100 ms, AF. Tunables in `ssd1306_conf.h`: `SH1107_DISPLAY_OFFSET` (0x00; try 0x60/0x20 if shifted), `SH1107_DCDC_SETTING` (0x8A; try 0x8B/0x81 if dark), `SSD1306_CONTRAST` (0xFF), and the mirror flags (the default keeps the original A1/C8 orientation). The SSD1306 path is kept under `#else` |
| D: audit | ✅ Verified (last phase) | Line-level comparison of every file in `D:\ST projects\Hope_V1` (minus Debug/.git) against the Desktop: no file exists only in D: (besides the renamed `.gitignore.txt`), and every line present only in D: is an intentional replacement from phases 1-4. D: has a single branch, no stash, HEAD `f4c46c8`. The Desktop is the complete superset |
| C4 | ✅ Done (phase 1) | `close_light_menu()`, `lightPrevOverlay`; MENU closes the light menu; the previous screen is drawn under the overlay |
| H1 | ✅ Done | (a) done by the user. (b) branches swapped: empty slot → setup, active → delete prompt. (c) the delete prompt is now a modal branch before the state switch (NEXT/PREV/PLAY/MENU), and the old post-switch block is removed. Added `ui_alarm_delete_cancel()` (renderer .c/.h) |
| H2 | ✅ Done | DHT22: PB8 is set back to an open-drain output on every read; release and capture-arm happen inside a critical section with no 30 µs delay; ISR falling branch uses `else if`; TIM4 priority set explicitly to 0. Also covers L10 |
| H3 | ✅ Done | `volume()` never saves a VOLUME overlay as `previousOverlay` |
| H4 | ✅ Done | (a) done by the user. (b) on alarm start, the light menu and volume pop-up are closed first; `previousOverlayAlarm` is saved and restored by `stop_Alarm()`; sleep-timer expiry doesn't send STOP while the alarm rings. (c) `check_alarm()` isn't polled while `UI_ALARM_FIRING` |
| H5 | ✅ Done | `i2c1_bus_recover()` (main.c USER CODE 4, prototype in main.h EFP): DeInit, 9 SCL clocks, STOP, Init. ssd1306: commands use a 20 ms timeout, the DMA start is checked, the semaphore wait has a 50 ms timeout, stale gives are cleared, `HAL_I2C_ErrorCallback` gives the semaphore, both write paths take `i2c_mutex`, and `UpdateScreen` abandons the frame on error. RTC: `get_RTC_Data()` returns NULL on failure (time_service keeps the last time), `set_RTC_Data()` returns bool and takes the mutex, `update_time` refreshes the cache on success. A plain NACK skips recovery. main.c checks that the semaphore and mutex were created. Note: if the OLED loses power it would need `ssd1306_Init()` again (not handled) |
| H6 | ✅ Verified, no change | The user confirmed an 8.000 MHz crystal, which matches PLLM=8 / HSE_VALUE 8 MHz |
| M1 | ✅ Done (phase 3) | Entering the song list no longer sends anything to the music queue |
| M2 | ✅ Done (phase 3) | NEXT/PREV send `EVT_PLAY` with the on-screen index + 1 and set `play_state = 1` |
| M3 | ✅ Done (phase 3) | `play()` uses 0x12 (`/MP3/NNNN.mp3`). The SD layout is documented in the README. Alarm tone = `/MP3/0026.mp3` |
| M4 | ✅ Done (phase 3) | Software loop instead of DFPlayer 0x19 (ignored by many clones). UART RX via `HAL_UART_Receive_IT` byte-by-byte, with a frame parser in DFPLAYER_driver.c. The 0x3D "finished" frame calls a registered callback (`df_set_finished_callback`), which `music_service.c` uses to post `EVT_TRACK_FINISHED` to `musicQueueHandle`. `music_Task` replays `loop_track` (set by EVT_PLAY, cleared by STOP/BLE_ON, suppressed while paused, 1 s debounce for duplicate reports). `HAL_UART_ErrorCallback` restarts RX. Unused `repeat()`/RX globals removed |
| M5 | ✅ Done (phase 3) | 60 ms gap after each DFPlayer command in `music_Task`. `FEEDBACK_BYTE` set to 0. TX queue enqueue/dequeue/start are now under `df_lock()` (ISR-aware critical section). A failed `HAL_UART_Transmit_DMA` releases `uart_tx_ready` |
| M6 | ✅ Done (phase 3) | `DMA_BUFFER_SIZE = TOTAL_BITS + 256` (320 µs reset); PulseFinished callbacks check TIM1 |
| M7 | ✅ Done (phase 3) | MENU cancels UI_TIMER (→ MAIN) and UI_TIMER_NOWPLAYING (→ player). The renderer draws the main/player screen under the timer pop-ups. Confirming the timer while a song is playing no longer restarts it. `saved_song` now uses `ui_nowplaying_get_index()` (the list cursor could differ). `play_state` is set when playback starts from the timer |
| M8 | ✅ Done (phase 3) | Light overlay box 8,2 to 119,125; list y=17, h=88 (8 rows) |
| M9 | ✅ Done (phase 3) | `audio_service_ble_enable()` stops the DFPlayer first. New dedicated `UI_DrawPlayDisplay_ble()` (icon, pulse ring, pairing hint, "MENU: exit") |
| M10 | ✅ Done (phase 3) | EXTI callback returns if `uiQueueHandle == NULL` |
| M11 | ✅ Done (phase 3) | BCD masks (0x7F/0x7F/0x3F) applied to `raw_data` before `BCDtoDeci()` |
| M12 | ✅ Done (phase 3) | Timer expiry sets `play_state = 0`, `song_stopped = 1`, `ui_nowplaying_set_playing(0)`. OK afterwards sends EVT_PLAY (restart) instead of EVT_RESUME. New renderer API: `ui_nowplaying_set_playing()`, `ui_nowplaying_get_index()` |
| M13 | ✅ Done (phase 3) | defaultTask deletes itself; music priority 3 (UI 2, light 1). `configCHECK_FOR_STACK_OVERFLOW 2` + `configUSE_MALLOC_FAILED_HOOK 1`, with hooks in freertos.c (USER CODE Application: DEBUG halts, release resets). Semaphore/mutex creation checked (phase 2). Heap, FPU and the hook settings are mirrored into `.ioc` FREERTOS params so CubeMX regeneration keeps them (the user's 20 KB heap was previously only in the .h). Not done: measuring stack high-water marks needs hardware |
| L1 | ✅ Done (phase 4) | `check_alarm()`: when now < last_now (midnight or clock moved back), all `triggered_today` are cleared. The old `now < alarm_time` reset is removed. Alarms aren't evaluated until `time_is_valid()`; the first valid time runs `alarm_service_resync()` (alarms already passed today are marked done). `alarm_service_time_changed()` is called after the user sets the clock |
| L2 | ✅ Done (phase 4) | New `AT24C32_driver.c/.h` (addr 0x57, 16-bit mem addr, page write + 10 ms write cycle, `i2c_mutex`, bus recovery). The alarm record is one 32-byte page at 0x0000: magic 0xA7, 10×{active,h,m}, ~checksum. Saved on add/delete; `alarm_service_load()` runs at the top of `ui_Task` after `ssd1306_Init()`. A blank or corrupt EEPROM leaves the alarms empty |
| L3 | ✅ Done (phase 4) | `get_sensor_data()` returns bool and keeps the last good values. New `ui_data.sensor_valid` field; the main screen shows `--C Hum:--%` until the first good read. Values are rounded instead of truncated |
| L4 | ✅ Done (phase 4) | `DEFAULT_VOLUME` (music.h) used by ui_task, music_service and `setVolume()` at UI start-up |
| L5 | ✅ Done (phase 4) | Hints: "TIMER : save" (time/alarm setup), "LIGHT:next MENU:ok", "TIMER:next OK:set", "Press any key" (alarm) |
| L6 | ✅ Done (phase 4) | `user_light_mode` is remembered and restored by `stop_Alarm()`. A playing lullaby is shown as stopped after an alarm (OK restarts it). The sleep timer is cancelled when an alarm fires |
| L7 | ✅ Done (phase 4) | `music.h` includes stdint; typo prototype removed; DFPlayer API renamed `df_*` and internals made static; unused RX globals removed; `WS2812_NUM_LEDS` shared; duplicate `MAX_SONGS` removed; file-local helpers made static; `(void)` prototypes; unused task params marked. `.gitignore.txt` renamed to `.gitignore` (+ `*.su`, `*.cyclo`) and `Debug/` untracked with `git rm -r --cached` (files remain on disk). README: DHT22, EEPROM alarms, SD layout, new directory tree, unclosed code fence fixed. `-Wall -Wextra` on user code is clean apart from the CubeMX `StartDefaultTask(argument)` |
| L8 | 🟡 Placeholder | Index 5 renamed "Lullaby 6" with a TODO. **The user needs to supply the real title of 0006.mp3** |
| L9 | ✅ Done (phase 4) | MENU in the Time submenu goes to UI_STATE_MENU |
| L10 | ✅ Done (phase 2) | Open-drain line + explicit TIM4 priority |
| L11 | ℹ️ Hardware notes only | No code change. The touch-sensor mode can't be checked physically; confirm on hardware (a key that responds only on every 2nd touch means toggle mode) |
| M14 | ✅ Done | `.cproject` Drivers/Services include paths changed to `${workspace_loc:/${ProjName}/...}`. `Hope_V1 Debug.launch` still logs to D:\ (harmless) |
| L3 / L7 | 🟡 Partial (user) | Shadowed `raw_humidity/raw_temp` globals removed from sensor_service.c |

Build check: a full compile and link of this folder with the CubeIDE GCC 14.3 toolchain (script in the session scratchpad mirrors the Debug flags with local include paths) gives 0 errors and 0 warnings in user code.

## 0. Project facts (context for the fixes)

- MCU: STM32F412RETx at 100 MHz (HSE → PLL), FreeRTOS via CMSIS-RTOS v2, heap_4, `configTOTAL_HEAP_SIZE = 15360`, tick = 1 kHz.
- Build: STM32CubeIDE, GCC 14.3, `-O0`, nano specs. `Debug/Hope_V1.elf` is newer than all sources, so the **current tree compiles and links** (no blocking syntax errors).
- Tasks (main.c:98-117): `ui_Task` (prio 2, 1000 words), `music_Task` (prio 2, 1000 words), `light_Task` (prio 1, 1000 words), plus the CubeMX `defaultTask` (osPriorityNormal = 24, 128×4 bytes).
- Queues: `uiQueueHandle` (ui_msg_t ×8), `musicQueueHandle` (music_msg_t ×8), `lightQueueHandle` (light_msg_t ×8). Binary semaphore `i2c_dma_sem` for the OLED DMA.
- Pin map (main.h):

| Function | Pin | Notes |
|---|---|---|
| OLED + DS3231 | I2C1 PB6/PB7 | shared bus, OLED uses DMA1_Stream1 ch0 |
| DFPlayer | USART1 PA9/PA10, 9600 | TX DMA2_Stream7, RX DMA2_Stream2 (RX unused) |
| WS2812 | TIM1_CH1 PA8 | DMA2_Stream1 ch6, circular, halfword |
| DHT22 | PB8 (TIM4_CH3, AF2) | bare-metal driver, owns `TIM4_IRQHandler` |
| BT power | PB9 `BLE_SWITCH` | GPIO high = on |
| Touch keys | PB12 MUSIC, PB13 TIMER, PB14 VOL_UP, PB15 DWN, PD2 LIGHT, PB3 UP, PB4 PLAY, PB5 VOL_DWN | EXTI rising, prio 5 |
| LED_TOGGLE | PB2 | configured, never used (PB2 = BOOT1) |

## 1. Implementation order

1. **C2** (I2C IRQs, CubeMX) → **C1** (call init) → **C3** (SH1107 init) → **H5** (I2C timeouts). Do the display path first, because nothing else can be seen until it works.
2. **C4, H3, H4, M7**: UI overlay/state bookkeeping. Refactor all of these together (section 3.1).
3. **H1**: alarms.
4. **H2**: DHT22.
5. **H6**: verify the crystal before tuning any timing.
6. Medium items M1–M14, then Low items L1–L11.

After each group: build, flash, and run the hardware checks listed with the item.

---

## 2. CRITICAL

### C1 — OLED never initialized
- **Where:** `Drivers/OLED/ssd1306.c:89` (`ssd1306_Init`). Nothing calls it. The linker map lists `.text.ssd1306_Init` under "Discarded input sections".
- **Effect:** the panel stays in its reset state (display off), so the screen is always blank.
- **Fix:**
  - In `Core/Src/Applications/ui_task.c`, at the very top of `ui_Task()` (line 143), before the `for(;;)`, add `ssd1306_Init();`.
  - It must run inside a task, after the scheduler has started: `ssd1306_WriteData` blocks on `i2c_dma_sem`, which needs the scheduler.
  - In `ssd1306_Init`, replace `HAL_Delay(100)` (line 94) with `vTaskDelay(pdMS_TO_TICKS(100))` (include `task.h`), so the CPU isn't busy-waited.
- **Depends on:** C2 (otherwise the first `ssd1306_UpdateScreen()` inside init hangs).
- **Verify:** the screen turns on and shows the main screen.

### C2 — I2C1 event/error interrupts not enabled → UI task blocks forever
- **Where:**
  - `Core/Src/stm32f4xx_hal_msp.c:99-140` (I2C1 MSP init): it configures DMA but doesn't enable `I2C1_EV_IRQn`/`I2C1_ER_IRQn`.
  - `Core/Src/stm32f4xx_it.c`: has no `I2C1_EV_IRQHandler` or `I2C1_ER_IRQHandler`.
  - `Drivers/OLED/ssd1306.c:23-27`: waits on the semaphore with `portMAX_DELAY`.
- **Root cause:** in the STM32F4 HAL, `I2C_DMAXferCplt()` (stm32f4xx_hal_i2c.c) does **not** call `HAL_I2C_MemTxCpltCallback` for master TX. It re-enables `I2C_IT_EVT | I2C_IT_ERR` and leaves BTF → STOP → callback to `HAL_I2C_EV_IRQHandler`. Without the EV IRQ in the NVIC:
  - the callback never runs,
  - the semaphore is never given,
  - `ui_Task` blocks forever on the first page of the first `ssd1306_UpdateScreen()`,
  - and the I2C handle stays `HAL_I2C_STATE_BUSY_TX`, so the DS3231 reads fail too.
- **Effect today:** `ui_renderer_update()` → `ssd1306_UpdateScreen()` runs on the first loop iteration. The UI task freezes after that: no button handling, no alarms, no sensor reads.
- **Fix (preferred, via CubeMX so it survives regeneration):**
  1. Open `Hope_V1.ioc` → Connectivity → I2C1 → NVIC Settings → enable "I2C1 event interrupt" and "I2C1 error interrupt".
  2. Set both preemption priorities to 5 (≥ `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`, because the callback calls `xSemaphoreGiveFromISR`).
  3. Regenerate. This produces, in `stm32f4xx_it.c`:
     ```c
     void I2C1_EV_IRQHandler(void) { HAL_I2C_EV_IRQHandler(&hi2c1); }
     void I2C1_ER_IRQHandler(void) { HAL_I2C_ER_IRQHandler(&hi2c1); }
     ```
     and `HAL_NVIC_SetPriority/EnableIRQ` calls in the MSP.
  4. Check that the user code in `main.c`/`stm32f4xx_hal_msp.c` survived (it all sits inside USER CODE blocks).
- **Verify:** use a breakpoint or a GPIO toggle in `HAL_I2C_MemTxCpltCallback`. It should fire 16 times per frame.

### C3 — Init sequence is for SSD1306/SH1106, display is SH1107 (verify the module first)
- **Where:** `Drivers/OLED/ssd1306.c:89-188`.
- **Problem lines:**
  - `0x20,0x00` sets horizontal addressing. On SH1107, 0x20/0x21 select page/vertical mode.
  - The multiplex setting is sent as `0xFF` then `0x3F` (lines 131-143, a "Luma SH1106" hack). SH1107 needs `0xA8, 0x7F` for 128 rows.
  - `0xDA,0x12` (com pins) doesn't exist on SH1107.
  - `0x8D,0x14` is the SSD1306 charge pump. SH1107 uses `0xAD, 0x8A/0x8B`.
  - Display offset `0xD3,0x00`: many 128×128 SH1107 modules need `0x60` (or `0x00`, depending on how the COM lines are wired).
- **Fix:** add an `#ifdef SSD1306_USE_SH1107` (define it in `ssd1306_conf.h`) init path:
  ```c
  ssd1306_WriteCommand(0xAE);             // display off
  ssd1306_WriteCommand(0xD5); ssd1306_WriteCommand(0x51); // clock div
  ssd1306_WriteCommand(0x20);             // page addressing mode
  ssd1306_WriteCommand(0x81); ssd1306_WriteCommand(0x4F); // contrast
  ssd1306_WriteCommand(0xAD); ssd1306_WriteCommand(0x8A); // DC-DC (0x8B on some modules)
  ssd1306_WriteCommand(0xA0);             // segment remap (A1 to mirror)
  ssd1306_WriteCommand(0xC0);             // COM scan dir (C8 to flip)
  ssd1306_WriteCommand(0xDC); ssd1306_WriteCommand(0x00); // display start line
  ssd1306_WriteCommand(0xD3); ssd1306_WriteCommand(0x60); // display offset (try 0x00 if shifted)
  ssd1306_WriteCommand(0xD9); ssd1306_WriteCommand(0x22); // pre-charge
  ssd1306_WriteCommand(0xDB); ssd1306_WriteCommand(0x35); // VCOMH
  ssd1306_WriteCommand(0xA8); ssd1306_WriteCommand(0x7F); // multiplex 128
  ssd1306_WriteCommand(0xA4);             // follow RAM
  ssd1306_WriteCommand(0xA6);             // normal (not inverted)
  ssd1306_WriteCommand(0xAF);             // display on
  ```
  `ssd1306_UpdateScreen()` (page address `0xB0+i` for i=0..15, column `0x00/0x10`, 128 bytes) already works in SH1107 page mode. Keep it.
- **Verify:** check the module's datasheet or seller page first, to confirm the controller and I2C address 0x3C. Then confirm on hardware that text is upright, not mirrored, and not shifted. Adjust A0/A1, C0/C8 and D3 to suit.

### C4 — Light menu traps the UI in a blank `UI_LIGHT_LIST` state
- **Where:** `Core/Src/Applications/ui_task.c:543-566` and `605-612`.
- **Trace:**
  1. Press LIGHT in state X: the switch does nothing, then the post-switch block sets `previousState = X` and `currentState = UI_LIGHT_LIST`.
  2. Press LIGHT again: the switch case `UI_LIGHT_LIST` cycles the mode, then the post-switch block runs **again** and sets `previousState = UI_LIGHT_LIST`.
  3. After 5 s: `currentState = previousState = UI_LIGHT_LIST`, overlay NONE.
  4. The renderer has no case for `UI_LIGHT_LIST`, so the screen is blank, and only LIGHT does anything (and it changes the light mode). The only way out is a reset.
- **Fix:**
  ```c
  if (msg.evt == EVT_BTN_LIGHT && currentState != UI_LIGHT_LIST) {
      previousState   = currentState;
      lightPrevOverlay = currentOverlay.type;   // new static overlay_type_t
      currentState    = UI_LIGHT_LIST;
      currentOverlay.type = OVERLAY_LIGHT_MENU;
  }
  if (msg.evt == EVT_BTN_LIGHT) {               // refresh timeout on every press
      lightOverlay_open_tick = osKernelGetTickCount();
      lightOverlay = 1;
  }
  ```
  On timeout: `currentState = previousState; currentOverlay.type = lightPrevOverlay;`.
  Also: in the `UI_LIGHT_LIST` case, MENU should close the overlay immediately (same restore) and PLAY should close it too. That also fixes the "OK:apply X:close" hint (L5).
- **Verify:**
  - From every screen: press LIGHT 1, 2 and 5 times, wait 5 s, and check you're back on the same screen.
  - From the timer overlay: you should return to the timer overlay.

---

## 3. HIGH

### 3.1 Suggested refactor for C4 / H3 / H4 / M7
All of these come from one problem: two variables (`currentState`, `currentOverlay.type`) are saved and restored independently by three timers (volume, light, alarm). The minimal fix is per item, below. A cleaner fix:
- Keep a single "modal stack" (depth 2 is enough) of `{state, overlay}` that is pushed when a modal opens (light, alarm) and popped when it closes.
- Handle the volume pop-up as a separate flag drawn on top, rather than as the overlay value.
- Run every timeout check outside the `xQueueReceive` success branch.

### H1 — Alarm feature cannot be used (three bugs)
1. **Context never registered.** `Core/Src/UI/ui_renderer.c:247-251`:
   ```c
   void UI_SetAlarmListContext(SoftwareAlarm *alarms, uint8_t max_alarms) {
       if (!backend_alarms) return;   // BUG: backend_alarms is NULL here, so it always returns
   ```
   Change to `if (!alarms) return;`.
   Today: `backend_max_alarms` stays 0, so the list draws no rows, `ui_alarms_list_navigate` returns early, and `ui_is_selected_alarm_empty()` always returns 1.
2. **Inverted branch.** `ui_task.c:294-303`: `if (ui_is_selected_alarm_empty())` opens the DELETE overlay. Swap the branches:
   - empty slot → `ui_alarm_setup_seed(); currentState = UI_STATE_ALARM_SETUP;`
   - active slot → `currentOverlay.type = OVERLAY_ALARM_DELETE;`
   Fix the comment too.
3. **Delete overlay confirms on the same press, and keys leak.** In `ui_task.c:568-578`, the overlay handler runs in the same iteration that opened it, so PLAY opens it and immediately calls `ui_alarm_delete_confirm()` (the default choice is NO, so nothing is deleted and the overlay closes at once). While the overlay is open, NEXT/PREV also move the alarm list cursor, and MENU leaves the overlay showing on the Time submenu.
   Fix: at the top of the non-alarm branch:
   ```c
   if (currentOverlay.type == OVERLAY_ALARM_DELETE) {
       if (msg.evt == EVT_BTN_NEXT)      ui_alarm_delete_navigate(1);
       else if (msg.evt == EVT_BTN_PREV) ui_alarm_delete_navigate(-1);
       else if (msg.evt == EVT_BTN_PLAY) { ui_alarm_delete_confirm(); currentOverlay.type = OVERLAY_NONE; }
       else if (msg.evt == EVT_BTN_MENU) { currentOverlay.type = OVERLAY_NONE; }   // cancel (choice reset inside confirm, or reset here)
       goto render;   // or wrap the state switch in else {}
   }
   ```
   Then delete the old block at lines 568-578.
- Optional: let PLAY on an active alarm open a small "Edit / Delete" choice, and seed the editor with the existing time instead of 08:00 (ui_renderer.c:290-294).
- **Verify:**
  - Add an alarm for 2 minutes ahead and check it shows in the list.
  - Wait for it: the alarm screen, tone (track 26) and red pulsing light should appear.
  - Press a key to dismiss.
  - Delete it with YES and check the slot is empty. Repeat with NO and check it's kept.

### H2 — DHT22 works once, then always TIMEOUT; start-signal race
- **Where:** `Core/Src/Drivers/DHT22_driver.c`.
- **Bug A (pin mode):**
  - `set_pin_output_low()` (line 91) carries the comment "PB8 already output". That's only true on the very first read, after `DHT22_init()`.
  - `set_pin_input()` switches PB8 to AF mode (MODER=10) and nothing switches it back.
  - On later reads the BSRR writes have no effect on the pin, so there's no start pulse, and every read hits the 10 ms TIMEOUT.
  - `get_sensor_data` then reports 0.0, so the screen shows `0C Hum:0%` from about the 10th second onward.
- **Bug B (race):**
  - After the 2 ms low, the code drives the pin HIGH and waits `delay_us(30)` before switching to capture (lines 97-100).
  - The DHT22 starts its 80 µs low response 20-40 µs after release, so the first falling edge is often missed.
  - Then the state machine treats the response-high rising edge as `WAIT_BIT_RISE`. Every bit shifts by one, giving a checksum error.
  - Task preemption between these steps (music_Task has the same priority) makes it worse.
- **Fix:**
  ```c
  static void dht_pin_output_od(void) {
      GPIOB->OTYPER |=  (1 << 8);                 // open-drain (bus has external pull-up)
      GPIOB->MODER  &= ~(3 << (8 * 2));
      GPIOB->MODER  |=  (1 << (8 * 2));           // output
  }
  void set_pin_output_low() {
      TIM4->DIER &= ~TIM_DIER_CC3IE;
      dht_pin_output_od();
      GPIOB->BSRR = (1 << (8 + 16));              // drive low
      delay_us(1200);                             // >=1 ms (DHT22 spec), still preemptible
      taskENTER_CRITICAL();
      // reset state machine BEFORE releasing the line
      current_state = WAIT_RESPONSE_LOW;
      memset((void*)data_buff, 0, 5); byte_index = bit_index = 0; capture_prev = 0;
      TIM4->SR &= ~TIM_SR_CC3IF;
      set_pin_input();                            // release line: pull-up takes it high, sensor answers
      TIM4->DIER |= TIM_DIER_CC3IE;
      taskEXIT_CRITICAL();
  }
  ```
  - Don't reset `TIM4->CNT` (capture uses differences anyway).
  - Use `else if` in the ISR falling branch for clarity.
  - Optionally give TIM4 an explicit priority (`NVIC_SetPriority(TIM4_IRQn, 4)`). It never calls FreeRTOS, so any priority is OK.
- Also (L3): in `sensor_service.c:36-40`, on error keep the previous good values (static cache) instead of writing 0.
- **Verify:** temperature and humidity change over several minutes (breathe on the sensor). Error count stays near zero; add a counter while debugging.

### H3 — Volume pop-up becomes permanent after two quick presses
- **Where:** `ui_task.c:88-110` and `582-589`.
- **Bug:** `previousOverlay.type = currentOverlay.type;` runs on every press. On the second press within 1.5 s, the current overlay is already VOLUME_UP/DOWN, so it gets saved as the "previous" one. The timeout then restores the volume pop-up, which stays on screen indefinitely.
- **Fix:**
  ```c
  if (currentOverlay.type != OVERLAY_VOLUME_UP && currentOverlay.type != OVERLAY_VOLUME_DOWN)
      previousOverlay.type = currentOverlay.type;
  ```
- Note: `volume()` calls both `setVolume()` (the renderer cache) and queues `EVT_SET_VOL`, which is correct. Only the UI task touches the DFPlayer queue, so there's no race.
- **Verify:** in Now Playing, press VOL+ 5 times quickly. The pop-up should close 1.5 s after the last press.

### H4 — Alarm doesn't auto-dismiss; other timers break the alarm screen
- **Where:** `ui_task.c:150-184`, `582-612`, `614-639`.
- **Bug A:** the 60 s timeout (`current_time - alarm_time >= ALARM_OVERLAY_PERIOD`) is inside `if (xQueueReceive(...) == pdPASS)`, so it's only checked when a key is pressed. Without a key, the device stays in `UI_ALARM_FIRING` and the light stays on `LIGHT_ALARM` forever.
- **Bug B:** if the alarm fires while the light overlay or volume pop-up is open, their timers later run `currentState = previousState` / `currentOverlay = previousOverlay`, which knocks the UI out of the alarm screen while the tone keeps playing. If the light overlay was open, `previousStateAlarm = UI_LIGHT_LIST`, and C4's trap comes back after the alarm.
- **Bug C:** `check_alarm()` is still polled while already firing. A second alarm in the same minute would overwrite `previousStateAlarm` with `UI_ALARM_FIRING`, and the device could never leave it.
- **Fix:**
  - Move the "stop alarm" code into a function `alarm_stop()`.
  - Call it from the key handler (any key), and also from a check placed **outside** the receive block: `if (currentState == UI_ALARM_FIRING && (now - alarm_time) >= pdMS_TO_TICKS(ALARM_OVERLAY_PERIOD)) alarm_stop();`.
  - When the alarm starts:
    - if `lightOverlay`, restore first (`currentState = previousState; lightOverlay = 0;`),
    - set `vol_flag = 0`,
    - only call `check_alarm()` when `currentState != UI_ALARM_FIRING`,
    - optionally cancel the sleep timer (`timer_flag = 0`) so it can't STOP the alarm tone.
- **Verify:**
  - Let an alarm ring without touching anything: it should stop after 60 s and return to the previous screen.
  - Open the light menu 2 s before the alarm: the alarm screen should stay up.

### H5 — No I2C timeouts or error handling (UI freezes on any bus glitch)
- **Where:** `Drivers/OLED/ssd1306.c:17-37`, `Core/Src/Drivers/DS3231_RTC_driver.c:65-87`, `Core/Src/Services/time_service.c:21-30`.
- **Fix:**
  ```c
  void ssd1306_WriteCommand(uint8_t byte) {
      HAL_I2C_Mem_Write(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x00, 1, &byte, 1, 20);
  }
  void ssd1306_WriteData(uint8_t *buffer, size_t buff_size) {
      if (HAL_I2C_Mem_Write_DMA(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x40, 1, buffer, buff_size) != HAL_OK) {
          i2c_recover(); return;
      }
      if (xSemaphoreTake(i2c_dma_sem, pdMS_TO_TICKS(50)) != pdTRUE) {
          HAL_I2C_Master_Abort_IT(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR); i2c_recover();
      }
  }
  void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
      if (hi2c->Instance == I2C1) { BaseType_t w = pdFALSE; xSemaphoreGiveFromISR(i2c_dma_sem, &w); portYIELD_FROM_ISR(w); }
  }
  ```
  - `i2c_recover()`: `HAL_I2C_DeInit(&hi2c1); HAL_I2C_Init(&hi2c1);`. Optionally clock out 9 SCL pulses if SDA is held low.
  - RTC: make `get_RTC_Data()` return `NULL` (or a status) when `HAL_I2C_Mem_Read != HAL_OK`. `split_time()` should keep the previous values on failure. Check `set_RTC_Data` too.
  - Since the RTC and OLED share I2C1 and are only used from `ui_Task`, no mutex is needed today. Add one (`i2c1_mutex`) if any other task ever touches the bus.
- **Verify:** pull the OLED SDA wire briefly while running. The UI should keep working (keys and audio respond) and the display should recover when it's reconnected.

### H6 — Check the HSE crystal frequency (Verify)
- **Where:** `Core/Src/main.c:249-256`, `Hope_V1.ioc` (`RCC.HSE_VALUE=8000000`), `stm32f4xx_hal_conf.h` `HSE_VALUE`.
- **Problem:** the configuration assumes 8 MHz (PLLM 8 → 1 MHz VCO input, ×200 = 200 MHz, /2 = 100 MHz). Many WeAct boards (e.g. the F411 "Black Pill") carry **25 MHz**. With 25 MHz:
  - VCO input = 3.125 MHz, VCO = 625 MHz (max 432 MHz), so the PLL is out of spec.
  - It may fail to lock (`HAL_RCC_OscConfig` → `Error_Handler`) or overclock.
  - Every baud rate and timer would be off: the DFPlayer link, WS2812 timing, and the DHT microsecond timer.
- **Fix:** read the crystal marking on the board. If it's 25 MHz, in CubeMX set the HSE input frequency to 25 MHz and PLLM=25, PLLN=200, PLLP=/2 (→ 100 MHz, APB1 /2 = 50 MHz, APB2 = 100 MHz), then regenerate.
- **Verify:** toggle a pin every 1 ms in a task and measure it with a scope or logic analyser, or check that the DFPlayer responds at 9600 baud.

---

## 4. MEDIUM

### M1 — Entering Music list sends a stale music command
- `ui_task.c:221-228`: only `music_msg.data = 0` is set, and `comm` keeps whatever it held last. On the first use after boot `music_msg` is all zeros, so it sends `EVT_PLAY` track 0. DFPlayer behaviour with track 0 is undefined; it may start playing.
- **Fix:** delete the `xQueueSend` (entering a list needn't touch audio), or set `music_msg.comm = EVT_STOP` if stopping is intended.

### M2 — NEXT/PREV audio and display get out of sync
- `ui_task.c:405-433` sends `EVT_NEXT`/`EVT_PREV` with the computed track in `data`, but `music_task.c:35-40` ignores `data` and sends DFPlayer's own NEXT (0x01) or PREV (0x02).
- DFPlayer next/prev follow the SD file order: they can land on track 26 (the alarm tone) and wrap at the file count, not at 25.
- **Fix:** in ui_task use `music_msg.comm = EVT_PLAY; music_msg.data = selected_song + 1;` for both directions. Also set `play_state = 1` and `np_is_playing = 1` (`ui_nowplaying_set` already does the latter).

### M3 — Play-by-index ignores file names
- `DFPLAYER_driver.c:19` `#define PLAY 0x03`. Command 0x03 plays the Nth file in FAT **copy order**, not `0005.mp3`. Copying files in a different order scrambles the song list.
- **Fix:** add `#define PLAY_MP3_FOLDER 0x12` and use it in `play()`. It plays `/MP3/NNNN.mp3` by name. Then document the SD layout in the README: folder `MP3`, files `0001.mp3`…`0025.mp3`, and `0026.mp3` = alarm tone (`ALARM_TONE` in ui_task.c:47).

### M4 — Songs never loop, so the sleep timer is mostly pointless
- After one track ends the DFPlayer stops, so a 60-min timer on a 3-min lullaby ends after 3 min. `repeat()` is declared in `DFPLAYER_driver.h:47` but never defined. `REPEAT_PLAY 0x11` is defined but unused.
- **Fix:**
  - Implement `void repeat_current(void)` using command `0x19` (single-loop on/off, param 0 = on), or use `0x08` (repeat a specific track).
  - Send it right after `play()` for lullabies. Not for the alarm tone (or deliberately loop the alarm until it's dismissed).
  - Add `EVT_PLAY_LOOP` to `comm_type_t` if both behaviours are needed.

### M5 — DFPlayer commands sent back-to-back
- The `music_Task` loop (music_task.c:26-56) sends commands as fast as the queue delivers. Sequences like STOP→PLAY, BLE_OFF→PLAY and PAUSE→… go out about 10 ms apart. The DFPlayer regularly drops commands that arrive within roughly 20-50 ms of each other, especially right after a PLAY.
- **Fix:** add `vTaskDelay(pdMS_TO_TICKS(50));` at the end of each handled message in `music_Task`. Optionally set `FEEDBACK_BYTE` to `0x00` (DFPLAYER_driver.c:28), since the ACK replies are never read (the RX DMA is configured but unused). Alternatively, implement RX parsing to catch the 0x3D "track finished" events and auto-advance.
- Also: `enqueue()` checks `is_full()` outside the critical section, and `dequeue()` from `df_try_start_tx()` runs unprotected in task context. It's safe today (one producer, and the ISR only fires when TX is busy), but wrap both in `taskENTER_CRITICAL`/`taskENTER_CRITICAL_FROM_ISR` for robustness.

### M6 — WS2812 reset gap too short
- `WS2812_driver.c:13`: `DMA_BUFFER_SIZE 464` gives 80 zero slots × 1.25 µs = 100 µs of low. Newer WS2812B (V5) and many clones need ≥280 µs, otherwise frames run together and the colours glitch or flicker.
- **Fix:** `#define DMA_BUFFER_SIZE (TOTAL_BITS + 256)` (= 640, even; HALF_SIZE = 320). That's 320 µs of reset.
- Add `if (htim->Instance != TIM1) return;` at the top of both `HAL_TIM_PWM_PulseFinished*Callback`s (lines 88-97).
- Optional: copy `leds[]` to a shadow buffer at frame start (when `streamIndex` wraps to 0) to avoid tearing while `light_Task` writes mid-frame.
- Note on CPU load: the DMA half-transfer IRQ fires about every 290 µs (232 slots) and computes 232 values. That's acceptable, but it's a constant background load at `-O0`.

### M7 — Timer overlay has no exit; Lights over the timer leaves a blank screen
- `ui_task.c:486-541`: `UI_TIMER` and `UI_TIMER_NOWPLAYING` only handle TIMER/PLAY (and volume in the second). There's no way to cancel. The renderer has no base screen for these states (default case draws nothing), so the overlay floats on black.
- Opening Lights from these states, then the timeout (`currentOverlay.type = OVERLAY_NONE`), leaves the state `UI_TIMER` with no overlay: a blank screen.
- **Fix:**
  - MENU in `UI_TIMER` → `currentState = UI_STATE_MAIN; currentOverlay.type = OVERLAY_NONE;`.
  - MENU in `UI_TIMER_NOWPLAYING` → back to `UI_STATE_NOWPLAYING_DF`.
  - Restore the saved overlay when the light menu closes (C4).
  - In the renderer, draw `UI_DrawMainScreen()` beneath `UI_TIMER` and `UI_DrawPlayDisplay_DF()` beneath `UI_TIMER_NOWPLAYING`.
  - Don't restart the current song when confirming in `UI_TIMER_NOWPLAYING` if it's the same track (optional).

### M8 — 8th light mode is never visible
- `ui_renderer.c:1026`: `draw_list(light_items, 8, light_selected, 0, 23u, 82u, 114u);`. 82/11 = 7 visible rows, top is fixed at 0, so "Night Fade" (index 7) is never drawn.
- **Fix:** list height 88 (8 rows, y 23..111) and move the footer line/hint to y 112/116 (or shrink the header). Alternatively pass a scroll top: `light_selected >= 7 ? light_selected - 6 : 0`.

### M9 — Bluetooth mode doesn't stop the DFPlayer; BT screen shows stale info
- `music_service.c:59-65` only toggles PB9. If a DF track is playing (e.g. after a timer start from Main), both sources come out of the amplifier. The README claims the music task "safely manages hardware transitions", but it doesn't.
- **Fix:** `audio_service_ble_enable() { stop(); vTaskDelay(pdMS_TO_TICKS(50)); BLE_Power_On(); }`.
- `ui_renderer.c:871-874`: the BT screen reuses the DF screen (last track name, "TRACK nn", play icon). Write a dedicated `UI_DrawPlayDisplay_ble()` with a large BT icon, "Bluetooth speaker" and "pair from your phone", plus the animation.

### M10 — Touch interrupt before queues exist → assert hang at boot
- EXTI is enabled in `MX_GPIO_Init()` (main.c:149, 485-498). `uiQueueHandle` is created later (main.c:179). `HAL_GPIO_EXTI_Callback` (ui_task.c:50-86) calls `xQueueSendFromISR(NULL, …)`, which trips `configASSERT` and spins with interrupts disabled. TTP223 outputs can glitch at power-up.
- **Fix:** add `if (uiQueueHandle == NULL) return;` at the top of the callback (and declare `uiQueueHandle` NULL-initialised, which it already is as a global).

### M11 — RTC hour mask applied after BCD conversion; I2C errors ignored
- `DS3231_RTC_driver.c:70-71`: `BCDtoDeci()` runs on the raw hour byte including bit 6 (12/24 h) and bit 5, and `& 0x3F` is then applied to the **decimal** value. This only works because 24 h mode keeps those bits at 0.
- **Fix:** `raw_data[2] &= 0x3F; raw_data[1] &= 0x7F; raw_data[0] &= 0x7F;` before `BCDtoDeci()`, and delete line 71.
- Also check the `HAL_I2C_Mem_Read` status (H5).
- Optional: clear the DS3231 OSF flag (reg 0x0F bit 7) after setting the time, and show "--:--" if OSF is set (time lost).

### M12 — Sleep-timer expiry leaves the UI saying "playing"
- `ui_task.c:591-603` sends `EVT_STOP` only.
- **Fix:** also set `play_state = 0`, set the renderer's paused flag (add `ui_nowplaying_set_playing(0)` rather than toggling blindly), and optionally send `LIGHT_NIGHT_FADE` or `LIGHT_OFF` to the light queue.
- Also define what MENU/back means after expiry.

### M13 — RTOS configuration hygiene
- `defaultTask` (main.c:68-72, 516-524) runs at `osPriorityNormal` (= 24), above every application task (1-2), and wakes every tick (`osDelay(1)`), causing 1000 needless context switches per second. **Fix:** replace its body with `vTaskDelete(NULL);`, or remove the task in CubeMX.
- Priorities: suggest music 3 (responsive audio), UI 2, light 1. Keep in mind that the DHT22 busy-wait (about 6 ms every 5 s) in the UI task delays light by that much.
- Heap: about 13.8 KB of 15 KB used. Each task uses TCB 168 B + 4000 B stack + 16 B heap headers, plus the queues and semaphore. Idle and timer tasks are static. That leaves about 1.5 KB margin.
  - Enable `configCHECK_FOR_STACK_OVERFLOW 2` and `configUSE_MALLOC_FAILED_HOOK 1`, and implement the hooks (e.g. blink LED_TOGGLE).
  - Measure with `uxTaskGetStackHighWaterMark` and trim the stacks (music/light likely need about 256-384 words).
  - Or raise the heap to about 24 KB (the F412 has 256 KB RAM).
- `i2c_dma_sem` creation isn't checked (main.c:169). Add `if (i2c_dma_sem == NULL) NVIC_SystemReset();`.

### M14 — Absolute include paths in the build
- `.cproject` contains `D:\ST projects\Hope_V1\Core\Src\Drivers` and `...\Services`. The generated `Debug/**/subdir.mk` also show `-I"D:/ST projects/Hope_V1/Core/Src/Applications"`, `.../Drivers/OLED`, `.../Core/Src/UI`. Building the Desktop copy will use headers from the D: copy (if it exists) or fail.
- **Fix:** in CubeIDE → Project Properties → C/C++ Build → Settings → MCU GCC Compiler → Include paths, replace each with a workspace-relative form (`../Core/Src/Drivers`, or `"${workspace_loc:/${ProjName}/Core/Src/Drivers}"`).

---

## 5. LOW

### L1 — Alarm at 00:00 fires only once ever
- `alarm_service.c:78-96`: the daily reset relies on `now < alarm_time`, which is never true when `alarm_time == 0`. Alarms near midnight also re-arm late.
- **Fix:** keep `static uint32_t last_now;`. If `now < last_now` (the day wrapped), clear `triggered_today` on every alarm. Then apply the existing trigger check with `alarm_time <= now`.

### L2 — Alarms lost on power cycle
- `my_backend_alarms_array` is RAM only. Optional: persist the alarms to the last flash sector (F412: sector 7, 128 KB, erase on write) or to an external EEPROM (many DS3231 modules carry an AT24C32 at 0x57 on the same I2C bus, which is the easiest option).

### L3 — Sensor error shows 0 °C / 0 %
- `sensor_service.c:36-40`. Keep the last valid values. Optionally show "--" after N consecutive failures.
- Also remove the unused globals `raw_humidity`/`raw_temp` (lines 15-16), which the locals shadow.

### L4 — Volume bar shows 0 at boot
- `ui_renderer.c:34` `static uint8_t volume = 0;` while `ui_task.c:32` `set_Vol = 5` and `music_service.c:25` `set_volume(5)`. Initialise to 5, or call `setVolume(set_Vol)` at the start of `ui_Task`. Define one `DEFAULT_VOLUME` constant used by all three.
- `music_service.c:15` `current_vol = 10` is misleading; set it to 5 or remove it.

### L5 — On-screen hints don't match the controls
- `ui_renderer.c:912` "Hold OK : confirm" and `:997` "HOLD : save": there's no long-press detection. Confirming is done with the TIMER key (ui_task.c:325, 350). Change the text to "TIMER : save".
- `:1030` "OK:apply X:close" and `:1048` "OK:set X:close": the light menu applies on LIGHT and there's no X key. Change the text to match the behaviour after C4/M7 (e.g. "LIGHT:next MENU:close", "TIMER:next OK:set").
- The alarm overlay says "Press OK to dismiss", but any key dismisses it. That's fine, or say "Press any key".

### L6 — Alarm dismissal side effects
- `ui_task.c:170` forces `light_msg.mode = 0` (off), so the user's previous light mode is lost. Save the last mode sent and restore it.
- If a DF song was playing when the alarm fired, it was replaced by the tone and isn't resumed. `play_state` still says 1. Either resume it (replay `saved` index) or set the state to paused.

### L7 — Code hygiene / warnings
- `music.h` uses `uint16_t` without `#include <stdint.h>`. It compiles only because of the include order.
- `music_service.h:14` `void auido_service_init();` is a typo duplicate. Delete it.
- `DFPLAYER_driver.h:47` `void repeat();` is declared but not defined (see M4).
- Unused: `df_rx_buf`, `df_old_pos`, `uart_rx_ready`, `playback_status`, `rx_buffer` (DFPLAYER_driver.c), `LED_TOGGLE` pin, `REPEAT_PLAY`.
- `DFPLAYER_driver.h` exports generic names `play/pause/stop/resume`. `pause()` clashes with POSIX `<unistd.h>` if that header is ever included, so prefix them (`df_play`, …).
- `NUM_LEDS` is defined in both `WS2812_driver.c` and `light_service.c`, and `MAX_SONGS` in both `ui_renderer.c` and `ui_renderer.h`. Keep each in a single header.
- `DS3231_RTC_driver.c` includes `FreeRTOS.h` needlessly.
- Use `(void)` prototypes (e.g. `void alarm_service_init(void);`) everywhere.
- Make file-local helpers `static` (`volume`, `timer_counter` in ui_task.c; `deciToBCD` etc.).
- `.gitignore.txt` is misnamed, so it's ignored by git and the whole `Debug/` build output (~234 files) is committed. Rename it to `.gitignore` containing `Debug/`, `Release/`, `*.o`, `*.d`, `*.su`, `*.cyclo`, then `git rm -r --cached Debug`.
- Git status shows the deleted `Core/Inc/music_service.h`, `Core/Src/UI/BitMaps.h` and `Core/Src/UI/ui_events.c`, and the untracked `WS2812_driver.*`. Commit them deliberately.
- README: says DHT11 in several places (sensor is DHT22). The directory tree is out of date. Document the SD-card layout (M3).

### L8 — Placeholder song name
- `ui_renderer.c:43` index 5 is `"  "`, so the list shows a blank row and the player shows a blank title. Give track 0006 a real name or remove it (and renumber the files).

### L9 — Inconsistent back navigation
- `ui_task.c:247-249`: MENU in the Time submenu goes to MAIN, while every other submenu goes back one level. Use `UI_STATE_MENU` for consistency (if that's the intended behaviour).

### L10 — DHT22 electrical/IRQ details
- The line is driven push-pull (DHT22_driver.c:38). Use open-drain (part of H2) so the MCU never fights the sensor.
- `NVIC_EnableIRQ(TIM4_IRQn)` without a priority means priority 0. That's fine because the ISR uses no FreeRTOS API, but set it explicitly and comment it.

### L11 — Hardware notes (not code bugs, worth checking)
- WS2812B powered at 5 V needs VIH ≥ 0.7×VDD = 3.5 V, but the STM32 drives 3.3 V, which is marginal. Add a 74AHCT125 level shifter, or power the first LED from about 4.3 V (diode drop).
- LED current: torch (255,180,80) and sunrise (255,255,63) ×16 LEDs draw about 0.7-0.8 A at 5 V. Size the supply and traces, or cap brightness in software.
- PB2 (`LED_TOGGLE`) is the BOOT1 pin. That's harmless at runtime, but don't load it heavily.
- TTP223 modules: set the jumpers to momentary, active-high (the default). Toggle mode would emit only every other edge.
- The DFPlayer TX/RX should have a 1 kΩ series resistor on the module RX (reduces noise and hum).
- The MH-MX8 BT module is powered via a transistor on PB9. Make sure the audio grounds of DFPlayer, BT and PAM8403 are star-connected, as the README says.

---

## 6. Final verification checklist (after all fixes)
1. A clean build in CubeIDE gives zero errors. Review the new warnings with `-Wall -Wextra`.
2. Boot: the OLED shows the main screen with the time, temperature/humidity and animation. The volume bar shows 5.
3. Every touch key responds on every screen, with no dead ends (MENU always backs out).
4. Light menu: cycle all 8 modes (including Night Fade). It auto-closes after 5 s back to the same screen and overlay.
5. Volume: rapid presses leave no stuck pop-up.
6. Music: play, pause/resume, next/prev match the displayed title (SD layout per M3). Songs loop (M4).
7. Sleep timer: 5 min stops audio and the UI shows paused.
8. Bluetooth: DF audio stops, the BT screen shows, and the phone can pair and stream. Leaving BT powers it off.
9. Alarm: add, list, fire (tone, screen, light), auto-dismiss at 60 s, dismiss by key, delete YES/NO.
10. DHT22 values update every 5 s for 30+ minutes.
11. Pull the OLED SDA briefly: the system survives (H5).
12. Run 1 h and check the stack high-water marks and free heap (`xPortGetFreeHeapSize`).
