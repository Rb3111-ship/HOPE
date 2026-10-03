# HOPE Lullaby Device — Code Review Summary

**Date:** 3 Oct 2026  **Scope:** all firmware you wrote (tasks, services, drivers, UI, display driver, startup and CubeMX configuration)

## ✅ Status (updated 3 Oct 2026): all phases done

Every issue below has been fixed in this folder (`C:\Users\whp27\Desktop\Hope_V1`), which is now the single, complete copy of the code. Your newer edits from `D:\ST projects\Hope_V1` are merged in, and a line-by-line comparison confirmed nothing from D: is missing. The project builds with **0 errors and 0 warnings**. Two things remain open:
- **L8:** the real name of song #6 (it's "Lullaby 6" for now).
- **Testing on the real board:** see the first-flash checklist at the end.

## The short version
- **Syntax:** the project **compiles and links**, so there are no syntax errors that stop the build. There are only minor warning-level and tidy-up items.
- **Logic:** I found **35 issues**: **4 Critical, 6 High, 14 Medium, 11 Low**, plus a few things to check on your hardware.
- **Biggest finding:** as the code stands, **the screen won't come on, and the screen task freezes on its first refresh**. That task also reads the buttons and checks alarms, so the device would look dead apart from the LED ring. The first four fixes below sort this out.

## Severity scale
| Level | Meaning |
|---|---|
| 🔴 **Critical** | The device doesn't work, freezes, or gets stuck until reset |
| 🟠 **High** | A main feature is broken |
| 🟡 **Medium** | Wrong behaviour in some situations, or fragile |
| 🟢 **Low** | Cosmetic, cleanup, or a small annoyance |
| 🔧 **Verify** | Depends on your exact hardware, so please check |

---

## 🔴 Critical
| # | Problem (plain words) | What you'd notice | Fix |
|---|---|---|---|
| C1 | The code never sends the "turn on" setup commands to the screen | Screen stays black | Call the screen's setup function when the UI task starts |
| C2 | One interrupt the screen relies on to say "finished sending" isn't switched on | The UI task waits forever: screen frozen, buttons and alarms dead | Enable the I2C1 event/error interrupts in CubeMX |
| C3 🔧 | The screen setup code is written for a different display chip (SSD1306), but yours is an SH1107 | Blank, garbled, or shifted picture | Replace it with SH1107 setup commands (check your module first) |
| C4 | Pressing the light button twice confuses the menu's "go back" memory | After 5 s you're stuck on a blank screen until you reset | Only remember the previous screen when first opening the light menu |

## 🟠 High
| # | Problem | What you'd notice | Fix |
|---|---|---|---|
| H1 | Three bugs together break alarms: the alarm list is never connected, the "empty slot" check is backwards, and the delete prompt confirms itself instantly | You can't add or delete any alarm | Fix the connection, swap the check, and make the delete prompt wait for a real choice |
| H2 | The temperature sensor's data pin is never switched back to "output" after the first reading, and the start signal is timed badly | Temperature/humidity show 0 after the first few seconds | Set the pin mode on every reading and tighten the start-signal timing |
| H3 | Pressing volume quickly twice makes the volume pop-up remember *itself* as the screen to go back to | The volume box never goes away | Don't remember the volume box as the "previous" screen |
| H4 | The alarm's 60-second auto-stop only gets checked when a button is pressed, and other pop-up timers can overwrite the alarm screen | The alarm and red light continue until a button is pressed; the alarm screen can vanish while it's still ringing | Check the timeout continuously and protect the alarm screen |
| H5 | Screen and clock communication waits forever if anything goes wrong | One loose wire or glitch freezes the whole interface | Add timeouts and error recovery |
| H6 🔧 | The clock setup assumes an 8 MHz crystal, but many WeAct boards have 25 MHz | If wrong: nothing times correctly (music player, LEDs, sensor) | Check the crystal and adjust the clock settings in CubeMX |

## 🟡 Medium
| # | Problem | What you'd notice | Fix |
|---|---|---|---|
| M1 | Opening the song list sends a leftover/garbage music command | A song might start playing on its own the first time | Remove that command |
| M2 | Next/Previous use the music player's own skip, which doesn't match the on-screen list | The title shown doesn't match what's playing; it can even skip to the alarm sound | Play the exact track number instead |
| M3 | Songs are picked by the order they were copied to the SD card, not by file name | The wrong song plays if files were copied out of order | Use the "play by file name" command and document the SD layout |
| M4 | Songs don't repeat | A 30/60-min sleep timer goes quiet after one 3-min song | Turn on loop mode for lullabies |
| M5 | Commands go to the music player too quickly, one after another | Commands get ignored sometimes (e.g. alarm or stop doesn't happen) | Leave a short gap between commands |
| M6 | The LED ring's "end of frame" pause is too short for newer LED chips | Flickering or wrong colours on some LED strips | Make the pause longer |
| M7 | No way to back out of the sleep-timer pop-up; opening lights on top of it leaves a blank screen | Stuck in the timer menu / blank screen | MENU cancels; restore pop-ups properly |
| M8 | The light menu only shows 7 of the 8 options | "Night Fade" can never be seen | Make the list one row taller |
| M9 | Turning on Bluetooth doesn't stop the SD music, and the Bluetooth screen shows the old song name | Two sounds at once; confusing screen | Stop the music first; draw a proper Bluetooth screen |
| M10 | A touch during power-up can happen before the system is ready | Rare freeze at boot | Ignore touches until ready |
| M11 | The clock's hour value is cleaned up in the wrong order | Wrong hour if the clock chip is ever in 12-hour mode | Clean it up before converting |
| M12 | When the sleep timer ends, the screen still says "playing" | Screen and reality disagree | Update the screen to "paused" |
| M13 | The RTOS setup has an unneeded background task with the highest priority, little spare memory, and no stack-overflow checks | Wasted CPU; hard-to-debug crashes later | Remove that task, enable the checks, size memory properly |
| M14 | Project build settings point to `D:\ST projects\Hope_V1`, not this folder | Building this copy may use the wrong files | Use relative paths in the project settings |

## 🟢 Low
| # | Problem | What you'd notice | Fix |
|---|---|---|---|
| L1 | An alarm set exactly for 00:00 only works once ever | Midnight alarm stops working | Reset alarms at the day change |
| L2 | Alarms are kept only in RAM | All alarms lost when unplugged | Save them (flash or the EEPROM on the RTC board) |
| L3 | A failed sensor reading shows 0 | Brief "0C 0%" readings | Keep the last good reading |
| L4 | The volume bar starts at 0 though the real volume is 5 | Wrong volume bar at power-on | Start it at 5 |
| L5 | Screen hints don't match the buttons ("Hold OK", "X:close") | Confusing instructions | Fix the text |
| L6 | Stopping the alarm turns the lights off and doesn't resume the song | Previous light/music setting lost | Restore the previous settings |
| L7 | General tidy-up: typos, unused code, a missing include, the misnamed `.gitignore.txt` (build files committed), README mentions DHT11 | Nothing on the device | Clean up |
| L8 | Song #6 has a blank name | Empty row in the song list | Name it or remove it |
| L9 | "Back" from the Time menu goes to Home rather than the Menu | Slightly odd navigation | Make it consistent |
| L10 | The sensor wire is driven in a less safe electrical mode | Nothing usually; small risk | Use open-drain mode |
| L11 | Hardware notes (see below) | — | — |

---

## Top 5 to fix first
1. **C2 + C1**: make the screen work and stop the UI freezing.
2. **C3**: correct the display setup for the SH1107.
3. **C4, H3, H4**: stop the menus and pop-ups from getting stuck.
4. **H1**: make alarms usable.
5. **H2**: make the temperature/humidity readings work continuously.

## Things to check on your hardware 🔧
- **Crystal on the board (H6):** read the marking on the small metal crystal: 8 MHz or 25 MHz?
- **Display chip (C3):** confirm the 1.5" OLED is really an SH1107 at I2C address 0x3C. The seller page or the back of the board usually says.
- **LED ring signal level:** the STM32 outputs 3.3 V but 5 V LEDs want about 3.5 V+. If the colours are flaky, add a level shifter (e.g. 74AHCT125).
- **LED power:** bright modes (Lamp, Sunrise) can draw about 0.8 A. Make sure your 5 V supply and wiring can handle it.
- **Touch sensors:** TTP223 boards should be in "momentary" (not toggle) mode.
- **SD card:** after fix M3, files go in a folder called `MP3`, named `0001.mp3` … `0025.mp3`, with the alarm sound as `0026.mp3`.

## First-flash checklist
1. In CubeIDE, import **this** folder (File → Import → Existing Projects), then Clean and Build.
2. Prepare the SD card: `MP3/0001.mp3` … `0025.mp3`, plus `0026.mp3` for the alarm.
3. **Screen.** It should show the clock, temperature/humidity (`--` for the first ~5 s) and the bunny animation. If it doesn't look right, edit `Drivers/OLED/ssd1306_conf.h`:
   - completely dark → set `SH1107_DCDC_SETTING` to `0x8B` (then `0x81`)
   - picture shifted or wrapped vertically → set `SH1107_DISPLAY_OFFSET` to `0x60` (then `0x20`)
   - upside down or mirrored → uncomment `SSD1306_MIRROR_VERT` and/or `SSD1306_MIRROR_HORIZ`
   - too bright at night → lower `SSD1306_CONTRAST` (e.g. `0x40`)
4. **Touch keys.** Each key should respond to every touch. If one only responds to every second touch, that sensor is in toggle mode.
5. Run through the rest: lights (all 8 modes, closes after 5 s), music (play/pause/next/prev, songs loop), sleep timer, Bluetooth, alarms (add, survives unplugging, rings, auto-stops after 60 s, delete), and temperature updating.

*The full technical details (exact lines and code changes) are in `CODE_REVIEW_FIXES.md`.*
