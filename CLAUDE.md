# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

An Arduino sketch for the **Adafruit Feather M4 Express (ATSAMD51)** + **Adafruit MIDI FeatherWing** that reads a triple piano-style sustain pedal unit connected via a TRS (1/4" stereo) jack and translates pedal state changes into MIDI Control Change messages sent to MIDI OUT.

**Platform constraint:** All code must be Arduino (C++). Do not suggest or use CircuitPython — it is too slow for low-latency MIDI requirements.

## Hardware

| Component | Detail |
|---|---|
| MCU board | Adafruit Feather M4 Express (ATSAMD51, 3.3V logic) |
| MIDI add-on | Adafruit MIDI FeatherWing Kit |
| USB host add-on | Adafruit USB Host FeatherWing with MAX3421E |
| Pedal input | Triple piano pedal unit — TRS 6.3mm (1/4") connector |

**MIDI FeatherWing** stacks directly on Feather headers:
- MIDI OUT → Feather `Serial1` TX (GPIO #1)
- MIDI IN → Feather `Serial1` RX (GPIO #0)
- Baud rate: 31250

**USB Host FeatherWing (MAX3421E)** stacks directly on Feather headers:
- Communicates via hardware SPI (MOSI=24, MISO=23, SCK=25)
- CS → GPIO #10, INT → GPIO #9
- Provides USB host port for connecting USB MIDI keyboards
- No conflict with MIDI FeatherWing (separate interfaces) or native USB Serial

## Triple-Pedal TRS Wiring

The pedal unit uses a **single-wire resistor-network** encoding for the center and left pedal — both pedal states are read from **one analog pin** via the Ring conductor. The right "Sustain / Damper" pedal uses a potentiometer to support half-pedaling and is read from the Tip conductor.

| TRS pin | Role |
|---|---|
| Tip (T) | Analog signal — Right pedal potentiometer, connect to one Feather analog pin (e.g. A2) |
| Ring (R) | Analog signal — Middle & left pedal switches, connect to another Feather analog pin (e.g. A3) |
| Sleeve (S) | 3.3V baseline power — connect to Feather's 3.3V pin through a series current-limiting resistor |

**Internal pedal circuit** (all three branches in parallel between Ring and Sleeve):

| Pedal | Position | Type | MIDI CC |
|---|---|---|---|
| Right | Sustain / Damper | Potentiometer — continuous, supports half-pedaling | CC 64 |
| Middle | Sostenuto | Fixed resistor + normally-open switch | CC 66 |
| Left | Soft (Una Corda) | Fixed resistor + normally-open switch | CC 67 |

Because two pedals share one analog line, pressing different pedals (or combinations) produces distinct voltage levels on the Ring conductor. **The specific resistor values must be measured** from the physical unit to establish accurate voltage thresholds.

Tip (the wiper) gives a proportional reading of right-pedal position independently. Testing confirmed the damper potentiometer does not affect the Ring baseline — Middle and Left pedal thresholds are stable.

Calibration / threshold discovery should be done in a dedicated sketch before encoding full MIDI logic.

CC value convention: 0–63 = off/released, 64–127 = on/pressed. The potentiometer (right/sustain pedal) can send proportional values 0–127 for half-pedaling rather than just binary.

## Arduino IDE Setup

1. **Board package**: Install *Adafruit SAMD Boards* via Boards Manager (requires *Arduino SAMD Boards* ≥ 1.6.11 first).
2. **Board selection**: Tools → Board → Adafruit Feather M4 Express (ATSAMD51)
3. **Port**: Tools → Port → the USB serial port that appears when the board is plugged in
4. **Compile**: Sketch → Verify/Compile (`Ctrl+R`)
5. **Upload**: Sketch → Upload (`Ctrl+U`)
6. **Serial monitor**: Tools → Serial Monitor (`Ctrl+Shift+M`) at 115200 baud — use for debug/calibration output

**Required libraries** — install via Library Manager:
- *MIDI Library* by Forty Seven Effects
- *Adafruit NeoPixel* by Adafruit
- *FlashStorage_SAMD* by Khoi Hoang — EEPROM emulation on SAMD51 internal NVM (the Feather M4 has no hardware EEPROM; do not use the standard `EEPROM.h`)
- *Adafruit TinyUSB Library* by Adafruit — USB host MIDI via MAX3421E (`CFG_TUH_MIDI` is already set to `CFG_TUH_DEVICE_MAX` in this library; no manual config edit required)

## Sketch Architecture

```
setup()
  init hardware MIDI — Serial1 @ 31250 baud via MIDI FeatherWing
  init USB host MIDI — TinyUSB + MAX3421E (CS=10, INT=9)
  configure A2 (Tip) and A3 (Ring) as analog inputs — no internal pull-up or pull-down
  (use external pull-down resistors to GND on each signal pin; Sleeve connects to 3.3V via external series resistor)

loop()
  tuh_task()                     // service TinyUSB host stack (must call every loop)
  raw = analogRead(A2)           // 12-bit: 0–4095 (Feather M4, not 10-bit AVR)
  decode pedal states from raw voltage thresholds
  for each pedal whose state changed since last loop:
    send CC via hardware MIDI (Serial1) and USB host MIDI simultaneously
  update last-known state
```

Key implementation notes:
- `Serial` (USB) is available for debug prints and does not conflict with `Serial1` (MIDI).
- The Feather M4 ADC is **12-bit** (0–4095). Do not assume 10-bit (0–1023) AVR defaults.
- The potentiometer is on a seperate line connected to the Tip conductor.
- The middle and left two switches interact on the same analog line (Ring conductor), so decoding requires understanding the full voltage range produced by each combination. A calibration sketch that prints raw ADC values while pressing each pedal combination is the recommended first step.
- Debounce the two switched pedals with `millis()`-based timing (not `delay()`) to keep the loop non-blocking.
- Define MIDI channel and all voltage thresholds as named constants at the top of the sketch.
