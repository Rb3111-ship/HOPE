# HOPE: Hardware Guide for the Single-Board Redesign

This guide describes the whole HOPE lullaby device so you can merge the two current boards (main board + power board) into **one board**. It covers:
- what every part does
- exactly what connects to what, pin by pin
- the upgrades that remove the buzz and the wiring mess
- how to route the board so it stays quiet

**The firmware does not need to change**, as long as every connection to the microcontroller uses the pins listed in [section 3](#3-microcontroller-pin-map-do-not-change). You can keep all your existing footprints and just move them around.

---

## Contents
1. [How the system works (overview)](#1-how-the-system-works-overview)
2. [Parts list](#2-parts-list)
3. [Microcontroller pin map (do not change)](#3-microcontroller-pin-map-do-not-change)
4. [Power](#4-power)
5. [Connections, module by module](#5-connections-module-by-module)
6. [The upgrades (what's different from the old boards)](#6-the-upgrades-whats-different-from-the-old-boards)
7. [Board layout: where to put things](#7-board-layout-where-to-put-things)
8. [Routing rules](#8-routing-rules)
9. [Connectors and cable management](#9-connectors-and-cable-management)
10. [Things that are easy to get wrong](#10-things-that-are-easy-to-get-wrong)
11. [Checklist before ordering](#11-checklist-before-ordering)
12. [Ordering from JLCPCB](#12-ordering-from-jlcpcb)
13. [Full connection table (netlist)](#13-full-connection-table-netlist)

---

## 1. How the system works (overview)

```text
                        ┌────────────────────── 5 V power in ───────────────────────┐
                        │                                                           │
                        ▼                                                           ▼
 ┌──────────┐  I2C   ┌──────────────────────┐   UART    ┌────────────┐   audio   ┌───────────┐    ┌──────────┐
 │  OLED    │◄──────►│                      │──────────►│  DFPlayer  │──────────►│           │    │ Speaker L│
 │ SH1107   │        │                      │◄──────────│ (SD card)  │   L / R   │  Audio    │───►│          │
 └──────────┘        │                      │           └────────────┘           │  mixer    │    └──────────┘
 ┌──────────┐  I2C   │   WeAct STM32F412R   │                                    │ (resistors)──►  HW-104
 │ DS3231   │◄──────►│   (the "brain")      │  on/off   ┌────────────┐   audio   │           │    PAM8403 amp
 │ RTC +    │        │                      │──────────►│ Bluetooth  │──────────►│           │    ┌──────────┐
 │ EEPROM   │        │                      │  switch   │  module    │   L / R   └───────────┘───►│ Speaker R│
 └──────────┘        │                      │           └────────────┘                            └──────────┘
 ┌──────────┐ 1-wire │                      │  data     ┌────────────┐
 │  DHT22   │◄──────►│                      │──────────►│ WS2812 LED │  (16 LEDs, own 5 V feed)
 └──────────┘        │                      │           │   ring     │
 ┌──────────┐ 8 wires│                      │           └────────────┘
 │ 8× TTP223│───────►│                      │
 │  touch   │        └──────────────────────┘
 └──────────┘
```

**In plain words:**
- **The microcontroller** (WeAct STM32F412R board) runs everything. It reads the touch buttons, draws the screen, keeps time, checks alarms, controls the lights and tells the music player what to play.
- **The DFPlayer Mini** plays MP3 files from its SD card. The microcontroller sends it commands over a serial (UART) link.
- **The Bluetooth module** turns the device into a Bluetooth speaker. The microcontroller only switches its power on and off.
- **The audio mixer** is just resistors. It combines the DFPlayer and Bluetooth audio into one left and one right signal for the amplifier.
- **The HW-104 (PAM8403) amplifier** drives the two speakers.
- **The OLED display** and **the DS3231 clock module** share one I2C bus (2 wires: SCL clock, SDA data). The clock module also carries a small **EEPROM** chip where the alarms are saved.
- **The DHT22** measures temperature and humidity.
- **The WS2812 LED ring** shows the light modes. It takes one data wire from the microcontroller and its own power feed.
- **The 8 TTP223 touch sensors** are the buttons. Each gives one wire that goes high (3.3–5 V) while it's touched.

---

## 2. Parts list

| Part | What it is | Notes |
|---|---|---|
| WeAct STM32F412R(ET6) core board | Microcontroller | Plugs in on two 2×15-way pin headers (the same footprint as now). It has its own USB-C, 3.3 V regulator and 8 MHz crystal |
| SH1107 1.5" 128×128 OLED, I2C | Display | Address 0x3C. **Check its pin order** ([section 10](#10-things-that-are-easy-to-get-wrong)) |
| DS3231 RTC module (with AT24C32 EEPROM) | Clock + alarm storage | RTC at 0x68, EEPROM at 0x57 |
| DFPlayer Mini | MP3 player | SD card: `MP3/0001.mp3` … (see the README) |
| Bluetooth audio module (HX-M18 / MH-MX8) | Bluetooth speaker | Uses only 5V, GND, L, R. Key and Mute are unused |
| HW-104 (PAM8403) | 2 × 3 W class-D amplifier | Bridged outputs, see [section 10](#10-things-that-are-easy-to-get-wrong) |
| WS2812B ring, 16 LEDs | Lights | Off-board, on a connector |
| DHT22 (AM2302) | Temp/humidity | Off-board or at the board edge near a vent |
| 8 × TTP223 touch boards | Buttons | Off-board, on connectors. Momentary mode (default, A/B pads open) |
| 2 × speakers, 4–8 Ω, ≤ 3 W | Sound | |
| **New parts for the upgrades** | | See [section 6](#6-the-upgrades-whats-different-from-the-old-boards) |
| 1 × P-MOSFET (AO3401A, SOT-23) *or* 1 × PNP (BC327, TO-92) | Bluetooth high-side switch | Replaces switching the module's ground |
| 1 × 10 kΩ | MOSFET gate pull-up | |
| 2 × 1 µF film or ceramic | Amp input coupling caps (in **series**) | Replace the old 100 nF to ground |
| 2 × 1 nF ceramic | Small RF filter after the mixer | Optional but recommended |
| 1 × 74AHCT1G125 (SOT-23-5) *or* 74HCT125 (DIP-14) | LED data level shifter, 3.3 V → 5 V | Optional; the ring works without it, but it's more reliable with it |
| Electrolytic caps: 1 × 1000 µF, 3 × 220–470 µF, 3 × 100 µF; all ≥ 10 V (16 V preferred) | Bulk decoupling | Placement is in [section 4](#4-power) |
| 100 nF ceramic × ~8 | Local decoupling, one per module | |
| Polyfuse 2.5–3 A (optional) | Input protection | |
| JST-XH (2.5 mm) connectors | All off-board wiring | Keyed, so they can't plug in backwards |

---

## 3. Microcontroller pin map (do not change)

These are the pins the firmware uses. **Wire the new board exactly like this** and the code runs unchanged. The "WeAct label" column is the name printed next to each header pin on the WeAct board.

| Function | MCU pin | WeAct label | Goes to | In-line parts |
|---|---|---|---|---|
| I2C SCL (clock) | **PB6** | B6 | OLED SCL + RTC SCL | none (the modules have pull-ups) |
| I2C SDA (data) | **PB7** | B7 | OLED SDA + RTC SDA | none |
| DFPlayer command out (UART TX) | **PA9** | A9 | DFPlayer **RX** | **1 kΩ** in series |
| DFPlayer replies in (UART RX) | **PA10** | A10 | DFPlayer **TX** | none |
| LED ring data | **PA8** | A8 | WS2812 DIN | **330 Ω** in series (+ optional level shifter) |
| DHT22 data | **PB8** | B8 | DHT22 DATA | **10 kΩ** pull-up to 3.3 V or 5 V |
| Bluetooth power switch | **PB9** | B9 | Bluetooth switch circuit | see [6.3](#63-bluetooth-power-switched-on-the-5-v-side) |
| Touch: UP (moves down / next) | **PB3** | B3 | Touch sensor 1 | — |
| Touch: PLAY / OK | **PB4** | B4 | Touch sensor 3 | — |
| Touch: VOLUME − | **PB5** | B5 | Touch sensor 2 | — |
| Touch: MENU / back | **PB12** | B12 | Touch sensor 5 | — |
| Touch: TIMER (also "save") | **PB13** | B13 | Touch sensor 6 | — |
| Touch: VOLUME + | **PB14** | B14 | Touch sensor 7 | — |
| Touch: DOWN (moves up / previous) | **PB15** | B15 | Touch sensor 8 | — |
| Touch: LIGHT | **PD2** | D2 (the unlabelled pin next to B3 on the old board) | Touch sensor 4 | — |
| *Spare*: status LED (optional) | PB2 | B2 | LED + 1 kΩ to GND | Already configured as an output in the code (`LED_TOGGLE`), currently unused |
| *Do not use*: SWD debug | PA13, PA14 | on WeAct's own SWD header | your ST-Link | Keep the WeAct SWD header reachable |

The "Touch sensor N" numbers match the pin order of the old 8-pin touch connector, so your sensors can stay wired the same way.

**If you ever must move a pin,** the firmware needs updating. The pins are defined in `Hope_V1.ioc` (CubeMX) and `Core/Inc/main.h`. Not all pins can do every job: I2C, UART, the LED timer (TIM1_CH1) and the DHT22 timer (TIM4_CH3) are tied to specific pins.

---

## 4. Power

### 4.1 Power budget (5 V)
| Load | Typical | Worst case |
|---|---|---|
| LED ring (16 × WS2812B) | 50–200 mA | **~0.8 A** (Lamp / Sunrise at full brightness) |
| Amplifier + speakers | 100–300 mA | **~1.2–1.5 A** peaks at high volume |
| DFPlayer | 20 mA | 50 mA |
| Bluetooth module | 30 mA | 60 mA |
| WeAct board (MCU) | 30 mA | 50 mA |
| OLED | 15 mA | 40 mA |
| 8 × touch sensors (with their LEDs) | 10 mA | 30 mA |
| RTC, DHT22 | < 5 mA | < 5 mA |
| **Total** | **~0.3–0.6 A** | **~2.5 A** |

**Use a 5 V supply rated at least 3 A.** A good USB-C PD charger or a 5 V 3 A adapter both work.

### 4.2 Power entry
```text
 5V IN ──► [polyfuse 3A] ──► [reverse-polarity protection, optional] ──┬──► 5V rail to everything
                                                                        │
                                                               1000µF ═╪═  +  100nF
                                                                        │
 GND IN ────────────────────────────────────────────────────────────────┴──► GND plane
```
- **Reverse-polarity protection** (optional but cheap insurance): a P-MOSFET in the + line, or a Schottky diode. A diode costs ~0.3 V.
- Put the **1000 µF** right at the power connector.

### 4.3 Decoupling: one set per module, as close as possible to its power pins
| Where | Capacitors |
|---|---|
| Power entry | 1000 µF + 100 nF |
| Amplifier VCC | 220–470 µF + 100 nF |
| LED ring connector (5 V to GND) | 220–470 µF + 100 nF |
| DFPlayer VCC | 100 µF + 100 nF |
| Bluetooth module 5 V | 100 µF + 100 nF |
| OLED connector | 100 µF + 100 nF |
| WeAct 5 V pin | 100 nF |
| RTC module, DHT22 | 100 nF each |

All of these go **between 5 V and GND, in parallel**. Electrolytic **+** goes to 5 V, the **stripe (−)** to GND.

### 4.4 3.3 V
The WeAct board makes its own 3.3 V. You only need 3.3 V on the PCB if you put the DHT22 pull-up there. Take it from the WeAct **3V3** header pin, and **don't** connect anything heavy to it.

---

## 5. Connections, module by module

**Notation:** "→ PB6" means wire it to the microcontroller pin labelled B6 on the WeAct board.

### 5.1 OLED display (SH1107, I2C)
| Display pin | Connect to |
|---|---|
| VCC | 5 V (via the 100 µF + 100 nF at the connector) |
| GND | GND |
| SCL | **PB6** (shared with the RTC SCL) |
| SDA | **PB7** (shared with the RTC SDA) |

> ⚠️ **The old main board had SDA and SCL swapped on this connector.** That's why the display needed its wires crossed. On the new board, label the connector pins by **what they connect to**, and match them to the pin names printed **on your display module**.

### 5.2 DS3231 RTC module (with EEPROM)
| RTC pin | Connect to |
|---|---|
| VCC | 5 V |
| GND | GND |
| SCL | **PB6** |
| SDA | **PB7** |
| SQW | not connected (spare) |
| 32K | not connected |

The OLED and RTC both sit on the same 2 wires. That's normal for I2C, since each device has its own address. **Do not add extra pull-up resistors**: both modules already have them.

### 5.3 DFPlayer Mini
| DFPlayer pin | Connect to | Notes |
|---|---|---|
| VCC | 5 V | 100 µF + 100 nF right at the pin |
| GND (both pins) | GND | |
| RX | **PA9** through **1 kΩ** | The 1 kΩ reduces noise and protects the pin |
| TX | **PA10** | Direct |
| DAC_L | Audio mixer, left | [section 6.2](#62-proper-audio-path-mixer--amplifier-input) |
| DAC_R | Audio mixer, right | |
| SPK1 / SPK2 | **not connected** | Its tiny built-in amp isn't used |
| BUSY | optional: a spare MCU pin, or leave unconnected | Goes low while playing. The code doesn't use it, but a pad lets you add it later |
| IO1, IO2, ADKEY1, ADKEY2, USB+/− | not connected | |

TX and RX **cross over**: the MCU's TX goes to the DFPlayer's RX and vice versa. The old board's silkscreen said "PA9" above the DFPlayer's TX pin, which was misleading. The copper was correct.

### 5.4 Bluetooth audio module
| BT pin | Connect to |
|---|---|
| 5V | **Switched 5 V** from the high-side switch ([6.3](#63-bluetooth-power-switched-on-the-5-v-side)) |
| GND | **GND plane, directly.** It is no longer switched |
| L | Audio mixer, left |
| R | Audio mixer, right |
| Key, Mute | not connected (pads optional) |

### 5.5 HW-104 / PAM8403 amplifier
| Amp pin | Connect to |
|---|---|
| VCC | 5 V (220–470 µF + 100 nF right at the pin) |
| GND (power) | GND plane |
| IN L+ | Mixer left, **through a 1 µF series capacitor** |
| IN GND | GND plane, **near the mixer** (this is the audio ground) |
| IN R+ | Mixer right, **through a 1 µF series capacitor** |
| OUT L+ / OUT L− | Left speaker connector (+ / −) |
| OUT R+ / OUT R− | Right speaker connector (+ / −) |

> ⚠️ **Never connect OUT L− or OUT R− to ground.** The PAM8403 is a "bridged" amplifier: both speaker wires carry signal. Grounding one can destroy it.

### 5.6 WS2812B LED ring
| Ring pin | Connect to |
|---|---|
| 5V | 5 V, with its **own thick trace from the power entry** (≥ 1 mm) and 220–470 µF + 100 nF at the connector |
| GND | GND, with a thick trace / plane back to the power entry |
| DIN | **PA8** → (optional level shifter) → **330 Ω** → DIN |

### 5.7 DHT22
| DHT22 pin | Connect to |
|---|---|
| 1 VCC | 3.3 V or 5 V (both work) |
| 2 DATA | **PB8**, plus a **10 kΩ pull-up** to the same supply as pin 1 |
| 3 | not connected |
| 4 GND | GND |

Place it **away from heat**: the LEDs, the amplifier and the WeAct regulator all warm up. Ideally put it at a board edge near a vent, or on a short cable.

### 5.8 Touch sensors (8 × TTP223)
Each sensor board has 3 pins: **VCC, GND, I/O**.

| Sensor | I/O goes to | Function |
|---|---|---|
| 1 | PB3 | UP (moves down / right / next song) |
| 2 | PB5 | Volume − |
| 3 | PB4 | PLAY / OK |
| 4 | PD2 | Light |
| 5 | PB12 | MENU / back |
| 6 | PB13 | Timer (also "save" in setup screens) |
| 7 | PB14 | Volume + |
| 8 | PB15 | DOWN (moves up / left / previous) |

- **VCC: 3.3 V is preferred.** The I/O then swings 0–3.3 V. The STM32 pins used here are 5 V tolerant, so 5 V also works; that's what the old board used.
- The firmware triggers on the **rising edge**, so the sensors must be in **momentary, active-high** mode. That's the default, with the A and B solder pads left open.

---

## 6. The upgrades (what's different from the old boards)

These fix the problems found while testing: the buzz, the muffled audio, the display wiring and the cable mess.

### 6.1 One solid ground plane
**The old problem:** the two boards were joined by power wires, and the audio had **no ground wire of its own**. Every current change (OLED pixels, LEDs, Bluetooth radio, even the touch sensors' LEDs) shifted the ground the amplifier listens to. You heard that as buzz.

**The fix:**
- Make the **entire bottom layer a ground pour**, cut as little as possible. Route signals on the top layer where you can.
- Also pour ground on the top layer in empty areas, and **stitch the two layers with vias** every ~10 mm, especially around the audio section.
- Everything connects to this plane. There's no separate "power board ground" any more.

### 6.2 Proper audio path (mixer → amplifier input)
```text
 DFPlayer DAC_L ──[1k]──┐
                        ├── MIX_L ──[ 1µF ]── IN L+  (HW-104)
 BT module L ─────[1k]──┘     │
                            [1nF]  (optional RF filter)
                              │
                             GND  ← connect this right next to the HW-104 "IN GND" pin

 (the right channel is identical: DAC_R and BT R → MIX_R → 1µF → IN R+)
```
- **The mixer:** one **1 kΩ** resistor from each source into a common node, as now.
- **The coupling capacitors (1 µF) go in SERIES** between the mixer and the amp input. On the old power board, the 100 nF capacitors went from the signal **to ground**. Combined with the 1 kΩ resistors, that cut the treble above ~1.6 kHz (muffled sound) without blocking DC.
- **The optional 1 nF from each mix node to ground** filters radio-frequency hash (Bluetooth, the OLED) without touching audio (cut-off around 160 kHz).
- **Keep the whole audio path short:** DFPlayer and Bluetooth module next to the amplifier, mixer parts in between, and the amp next to the speaker connectors.

### 6.3 Bluetooth power switched on the 5 V side
**The old problem:** the 2N2222 switched the module's **ground**. When off, the module's ground floated; when on, it bounced with the radio's current pulses. Meanwhile its audio outputs stayed connected to the mixer, which caused loud buzz in Bluetooth mode.

**The fix:** keep the module's GND solidly on the ground plane and switch its **+5 V** instead. The control logic stays the same: **PB9 high = Bluetooth on**, so the firmware needs no change.

**Option A: P-MOSFET (recommended).** It reuses your 2N2222 as the driver:
```text
            5V ─────────┬──────────────┐
                        │              │ S (source)
                      [10k]          ┌─┴─┐
                        │      G     │   │  AO3401A (P-channel MOSFET, SOT-23)
                        ├────────────┤   │
                        │            └─┬─┘
                        │              │ D (drain)
                    C ┌─┘              └────────► BT module 5V  (+ 100µF + 100nF to GND)
 PB9 ──[1k]──────── B │  2N2222
                      └─┐ E
                        │
                       GND          (also 10k from PB9 to GND, so BT stays off during reset)
```
- PB9 high → the 2N2222 turns on → it pulls the MOSFET gate low → the MOSFET turns on → the module gets 5 V.
- PB9 low (or the MCU in reset) → the 10 kΩ holds the gate at 5 V → the MOSFET is off.

**Option B: PNP transistor (through-hole only).** Same circuit, but replace the MOSFET with a **BC327** (or S8550): emitter to 5 V, collector to BT 5 V, and the base to the 2N2222's collector through **1 kΩ**. Add **10 kΩ** from base to emitter. It drops ~0.1–0.2 V, which the module doesn't mind.

### 6.4 Display connector wired correctly
SCL → PB6, SDA → PB7. Match the connector to the **real pin order printed on your display module** ([section 10](#10-things-that-are-easy-to-get-wrong)).

### 6.5 Decoupling at every module
The table is in [section 4.3](#43-decoupling-one-set-per-module-as-close-as-possible-to-its-power-pins). These are the capacitors you were going to add by hand, now built in.

### 6.6 LED ring data level shifter (optional, recommended)
The STM32 outputs 3.3 V, but a 5 V WS2812B officially wants ≥ 3.5 V. It usually works anyway, but marginally. A one-gate buffer makes it solid:
```text
 PA8 ──► 74AHCT1G125 (A in, Y out, /OE to GND, VCC = 5V) ──[330Ω]──► LED ring DIN
                      100nF from its VCC to GND, right at the chip
```
**74HCT125** (DIP-14) works too: use one gate, tie its /OE to GND, and tie the unused inputs to GND. If you skip the shifter, just keep the **330 Ω**.

### 6.7 Keyed connectors for everything off-board
See [section 9](#9-connectors-and-cable-management).

### 6.8 Nice extras (optional)
- **A status LED** on PB2 (via 1 kΩ to GND). It's already set up as an output in the firmware.
- **Test points** (small pads) on: 5V, 3V3, GND, SCL, SDA, PA9, PA10, PA8, MIX_L, MIX_R. They make probing with a multimeter easy.
- **A BUSY pad** from the DFPlayer to a spare MCU pin, for future use.
- **M3 mounting holes** where the enclosure's screw posts are.

---

## 7. Board layout: where to put things

Think of the board as **three zones** around the power entry:

```text
 ┌───────────────────────────────────────────────────────────────┐
 │  ZONE 1: POWER + HIGH CURRENT          (near the power input) │
 │  power jack, polyfuse, 1000µF, LED ring connector,            │
 │  amplifier (HW-104), speaker connectors                       │
 ├───────────────────────────────────────────────────────────────┤
 │  ZONE 2: AUDIO SOURCES                  (right next to the amp)│
 │  DFPlayer, Bluetooth module, mixer resistors, 1µF caps,       │
 │  BT high-side switch                                          │
 ├───────────────────────────────────────────────────────────────┤
 │  ZONE 3: DIGITAL                        (furthest from the amp)│
 │  WeAct STM32 board, OLED connector, RTC module,               │
 │  DHT22, touch connectors, LED level shifter                   │
 └───────────────────────────────────────────────────────────────┘
```

**For a round board,** put the power entry and amplifier on one side, the WeAct board on the opposite side, and the audio sources between them. Speaker connectors go next to the amp; touch connectors go around the digital side, towards where the sensors sit in the case.

**Placement rules:**
- **The amplifier goes near the speaker connectors and the power entry.** It draws the biggest current pulses, so keep that current loop small.
- **Keep the LED ring's power path short and separate,** running from the power entry straight to the LED connector, not past the audio.
- **Put the DFPlayer and Bluetooth module next to the amp,** so the audio wires (MIX_L, MIX_R) are a few centimetres at most.
- **Keep the WeAct board, the OLED connector and the touch connectors away from the audio area.**
- **Keep the DHT22 away from the amp, the LEDs and the WeAct regulator.**
- **Leave the WeAct USB-C port and SWD pins reachable** through the case, for flashing and debugging.
- **Make the DFPlayer's SD card slot accessible,** or at least removable without unscrewing everything.

---

## 8. Routing rules

### 8.1 Track widths (1 oz copper)
| Net | Width |
|---|---|
| 5 V main (entry → splits) | **≥ 1.5 mm**, or a pour |
| 5 V to the amp and to the LED ring | **≥ 1.0 mm** |
| 5 V to the other modules | 0.5 mm |
| GND | **plane** (don't route ground as thin tracks) |
| Signals (I2C, UART, touch, LED data, DHT22) | 0.25–0.3 mm |
| Audio (MIX_L, MIX_R, amp inputs) | 0.3–0.5 mm, short |
| Speaker outputs (OUT±) | 0.8–1.0 mm, each + and − pair routed side by side |

### 8.2 Keeping it quiet
1. **Never cut the ground plane under the audio section.** If a track has to cross the bottom layer there, keep it as short as possible, and put stitching vias on both sides.
2. **Route audio traces away from noisy ones:** the LED data (PA8), I2C (PB6/PB7), UART (PA9/PA10) and the speaker outputs. Where they must cross, cross at **90°**.
3. **Route each speaker's + and − next to each other** (a tight pair), straight from the amp to its connector.
4. **Return current follows the track above it.** With a solid plane underneath, every signal gets a clean return path automatically. That's why the plane matters so much.
5. **Decoupling caps go right at the pins** (within ~5 mm), with short fat connections to 5 V and to a ground via.
6. **Keep the touch sensor tracks away from the amplifier and speaker tracks.** Capacitive sensors pick up noise easily.
7. **The amp's "IN GND" pin connects to the plane right next to the mixer.** That's the audio reference point. The amp's power GND also goes straight into the plane.

### 8.3 I2C
- The OLED and RTC share SCL/SDA. Route them as a pair, as short as practical, with **no extra pull-ups** (the modules have them).
- Total bus length under ~30 cm, including the display cable.

---

## 9. Connectors and cable management

Use **JST-XH 2.5 mm** (through-hole, keyed) for everything that leaves the board, and print **the signal names and pin 1** on the silkscreen next to each connector.

| Connector | Pins | Pin order (suggested) |
|---|---|---|
| Power in | 2 (or USB-C) | 5V, GND |
| Speaker L | 2 | OUT L+, OUT L− |
| Speaker R | 2 | OUT R+, OUT R− |
| LED ring | 3 | 5V, DIN, GND |
| Display | 4 | **match your display module's order** (e.g. GND, VCC, SCL, SDA) |
| DHT22 (if off-board) | 3 | VCC, DATA, GND |
| Touch sensors | 8 × 3-pin (VCC, I/O, GND), **or** 1 × 10-pin (VCC, GND, T1…T8) | One 3-pin each keeps every sensor's cable separate and swappable. A 10-pin is tidier if the sensors are grouped |
| RTC | soldered on the board (or 4-pin: VCC, GND, SCL, SDA) | |

**Tips:**
- **Group cables by destination** (top of the case, sides, front) and tie or sleeve each group.
- **Cut each cable to length.** The keyed connectors mean nothing can go in backwards.
- **Twist each speaker pair loosely,** and keep the speaker wires away from the touch sensor wires.

---

## 10. Things that are easy to get wrong

1. **The display's pin order.** SH1107 modules come in different orders: GND-VCC-SCL-SDA, VCC-GND-SCL-SDA, and others. **Check the labels on your module** and copy that order onto the connector. A reversed VCC/GND can kill the display.
2. **TX/RX must cross:** MCU TX (PA9) → DFPlayer RX; DFPlayer TX → MCU RX (PA10).
3. **The amplifier outputs are bridged.** Never tie OUT− to ground, and never join the left and right outputs.
4. **The DS3231 module and coin cells.** Many DS3231 modules (e.g. "ZS-042") have a charging circuit meant for a rechargeable **LIR2032**. With a normal **CR2032**, the charging circuit can damage the battery. Either use an LIR2032, or remove the module's small diode (or its 200 Ω resistor) next to the battery holder.
5. **Electrolytic polarity:** + to 5 V, the stripe to GND.
6. **The WeAct footprint:** use your existing footprint (2 × 2×15 headers, same spacing). Check pin 1 and the orientation against the real board before ordering. The USB-C end is marked on the old silkscreen.
7. **PD2** is the unlabelled header pin next to B3 on the WeAct board. It's the LIGHT touch input.
8. **Touch sensors must be in momentary mode** (A/B pads open). In toggle mode, a key only responds to every second touch.
9. **Don't power anything heavy from the WeAct's 3V3 pin.** Its regulator is small.
10. **Leave the DFPlayer's SPK1/SPK2 unconnected.** They're its own little 3 W amp output. Using both it and the HW-104 would fight.

---

## 11. Checklist before ordering

- [ ] Every MCU connection matches the [pin map](#3-microcontroller-pin-map-do-not-change)
- [ ] Display connector pin order matches **your display module**
- [ ] Bluetooth module GND goes straight to ground; its 5 V is switched (high-side)
- [ ] 1 µF caps are in **series** with the amp inputs (not to ground)
- [ ] Amp OUT− pins are **not** connected to ground
- [ ] Decoupling caps are placed at every module, + to 5 V
- [ ] Bottom layer is a solid ground pour, stitched to the top layer
- [ ] No thin tracks on 5 V to the amp or the LED ring
- [ ] Mounting holes line up with the enclosure screw posts
- [ ] USB-C (WeAct), the SD card slot and the power input are reachable in the case
- [ ] Every connector has labels and a pin-1 mark on the silkscreen
- [ ] **DRC** (design rule check) passes with no errors
- [ ] You've checked the **3D view**: no parts overlapping, modules fit in the case height
- [ ] **Printed the board at 1:1 on paper** and laid it in the enclosure: holes, connectors and openings all line up

---

## 12. Ordering from JLCPCB

- **Shape:** any shape is fine, **round included**. JLCPCB takes the outline from the board-outline layer, which must be one **closed** shape. Price is based on the bounding rectangle; up to **100 × 100 mm** is the cheapest tier. Round boards are milled, which is normal.
- **Typical settings:** 2 layers, 1.6 mm thickness, 1 oz copper, HASL lead-free (or ENIG), any colour.
- **Upload the Gerber zip** that EasyEDA (or KiCad) exports. The online viewer shows the result before you pay. Check the outline, the drill holes and the silkscreen there.
- **SMD parts** (e.g. the AO3401A MOSFET or a 74AHCT1G125) can be hand-soldered with a fine tip, or added through JLCPCB's assembly service. Through-hole alternatives are given above if you prefer to solder everything yourself.

---

## 13. Full connection table (netlist)

Every electrical connection on the merged board. "Net" is a name you can give the wire in your PCB tool.

| Net | Connects |
|---|---|
| **+5V** | Power in +, polyfuse out, 1000 µF +, WeAct 5V pin, OLED VCC, RTC VCC, DFPlayer VCC, HW-104 VCC, LED ring 5V, DHT22 VCC (if on 5 V), touch VCC (if on 5 V), BT-switch MOSFET source (or PNP emitter), 10 kΩ gate pull-up, all decoupling caps + |
| **+5V_BT** | MOSFET drain (or PNP collector), BT module 5V, 100 µF + 100 nF |
| **+3V3** (optional) | WeAct 3V3 pin, DHT22 VCC + its 10 kΩ pull-up (if on 3.3 V), touch VCC (if on 3.3 V) |
| **GND** | Plane: everything's ground, including the BT module GND, HW-104 power GND and IN GND, DFPlayer GND, WeAct GND, all connectors' GND, all cap −, 2N2222 emitter |
| **I2C_SCL** | PB6, OLED SCL, RTC SCL |
| **I2C_SDA** | PB7, OLED SDA, RTC SDA |
| **DF_RX** | PA9 → 1 kΩ → DFPlayer RX |
| **DF_TX** | DFPlayer TX → PA10 |
| **LED_DIN** | PA8 → (level shifter) → 330 Ω → LED ring DIN |
| **DHT_DATA** | PB8, DHT22 DATA, 10 kΩ pull-up |
| **BT_EN** | PB9 → 1 kΩ → 2N2222 base; 10 kΩ PB9 → GND |
| **BT_GATE** | 2N2222 collector, MOSFET gate, 10 kΩ to +5V |
| **MIX_L** | DFPlayer DAC_L → 1 kΩ; BT L → 1 kΩ; joined node; 1 nF to GND; 1 µF to HW-104 IN L+ |
| **MIX_R** | DFPlayer DAC_R → 1 kΩ; BT R → 1 kΩ; joined node; 1 nF to GND; 1 µF to HW-104 IN R+ |
| **SPK_L+ / SPK_L−** | HW-104 OUT L+ / OUT L− → left speaker connector |
| **SPK_R+ / SPK_R−** | HW-104 OUT R+ / OUT R− → right speaker connector |
| **TOUCH_1** | PB3 ← sensor 1 I/O (UP) |
| **TOUCH_2** | PB5 ← sensor 2 I/O (VOL −) |
| **TOUCH_3** | PB4 ← sensor 3 I/O (PLAY/OK) |
| **TOUCH_4** | PD2 ← sensor 4 I/O (LIGHT) |
| **TOUCH_5** | PB12 ← sensor 5 I/O (MENU) |
| **TOUCH_6** | PB13 ← sensor 6 I/O (TIMER) |
| **TOUCH_7** | PB14 ← sensor 7 I/O (VOL +) |
| **TOUCH_8** | PB15 ← sensor 8 I/O (DOWN) |
| **STATUS_LED** (optional) | PB2 → LED → 1 kΩ → GND |

*Unconnected on purpose:* DFPlayer SPK1/SPK2, IO1/IO2, ADKEY1/2, USB±, BUSY (optional pad); BT Key/Mute; RTC SQW/32K.
