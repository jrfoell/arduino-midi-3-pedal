# Next Steps

## ✅ 1. Measure resistor values

Resistance measured across pedal connector:

- Right pedal only (damper / potentiometer) — 0–12kΩ range
- Middle pedal only (sostenuto) — 10kΩ
- Left pedal only (soft / una corda) — 5.6kΩ
- Middle + Left together — 3.6kΩ (matches parallel calculation)

## ✅ 2. External resistors required

The Sleeve conductor is the 3.3V power rail — connect it to the Feather's 3.3V
pin through a small series resistor (R1, **330Ω** or similar) for fault
protection. Keep R1 small: because it sits in series with the internal pedal
resistors and the pull-down, a large R1 compresses all the voltage levels
together and makes them hard to distinguish.

The Tip (A2) and Ring (A3) signal pins each need an external **pull-down
resistor to GND** (start with **10kΩ** each). Without pull-downs the analog
pins float and readings will be meaningless. With Sleeve at 3.3V, pressing a
pedal pulls the signal pin up through the internal pedal resistor, forming a
voltage divider with the pull-down. Do not use pull-ups on the signal pins —
with Sleeve already at 3.3V, a pull-up would put both ends of the divider at
the same voltage and eliminate all differentiation.

**Warning — do not connect the pedal to the Feather while it is also plugged
into the piano.** Digital pianos typically supply 3.3–5V through the pedal
jack. The Feather M4's analog pins are not 5V tolerant; feeding 5V into an
analog pin will damage the board. Only connect the pedal to the Feather with
the piano disconnected.

## ✅ 3. Account for potentiometer baseline shift in threshold logic

The Right/sustain pedal is a potentiometer whose body spans Ring and Sleeve —
so its position continuously shifts the Ring baseline voltage even when not
deliberately pressed. The decode logic for the Middle and Left pedals will need
to handle this, likely by detecting a relative voltage *change* from the current
baseline rather than comparing against fixed absolute thresholds. Tip (the
wiper) is read separately to get the right-pedal position.

## ✅ 4. Initial ADC readings confirmed

First bench test (wired per schematic). These values are encoded as compile-time
defaults used when no EEPROM calibration has been saved:

| Pedal state | Tip (A2) | Ring (A3) |
|---|---|---|
| All released | — | ~60 |
| Damper fully pressed | ~1850–1900 | — |
| Damper at rest | ~3900–4000 | — |
| Middle only | — | ~1900–2000 |
| Left only | — | ~2400–2500 |
| Middle + Left | — | ~2800–2900 |

Note: Tip reads **high at rest and low when pressed** (pot wiper sits near the
Sleeve/3.3V end when damper is released; moves toward Ring/GND end when pressed).

## 5. Code architecture

- **All settable constants** (pins, MIDI channel, defaults, timing) live in
  `arduino-midi-3-pedal.ino` for easy access.
- **Related functionality** is split into header files included by the .ino:
  - `calibration.h` — `CalibrationData` struct, EEPROM read/write,
    NeoPixel helpers, `runCalibration()`, `checkCalibrationButton()`
  - Future: `pedal_decode.h` — voltage-to-state decode logic
  - Future: `midi_output.h` — MIDI CC send helpers
- Goal: keep the main .ino under ~80 lines; no 1000-line files.

## 6. Calibration sequence (implemented in calibration.h)

Triggered by holding the left or middle pedal (or both) when the M4 powers on.
The Feather M4 Express has no user-programmable button; the reset button is not
available for user code. Calibration is checked once in `setup()` immediately
after calibration data loads.
Right-to-left pedal order:

| LED | Action |
|---|---|
| Solid blue | Release all pedals — reads baseline (2 s) |
| Slow blink violet → fast blink violet | Press + hold right / damper pedal, release when fast |
| Slow blink red → fast blink red | Press + hold middle / sostenuto pedal, release when fast |
| Slow blink orange → fast blink orange | Press + hold left / soft pedal, release when fast |
| Slow blink yellow → fast blink yellow | Press + hold middle + left together, release when fast |
| Blink green | Writing to EEPROM — then off |

On power-up, EEPROM is checked for a valid magic number (`0xFEED1234`). If
absent or corrupt, compile-time defaults are used automatically.

## 7. Next: MIDI output and pedal decode logic

- Read Ring (A3) → decode middle/left switch states using thresholds derived
  from calibration data (`threshNoneToMiddle`, `threshMiddleToLeft`,
  `threshLeftToBoth`).
- Read Tip (A2) → map damper position to CC 64 value 0–127, accounting for
  inverted range (high at rest, low when pressed).
- Debounce middle and left switches with `millis()`-based timing.
- Send CC 64 (damper), CC 66 (sostenuto), CC 67 (soft) on state changes via
  Serial1 @ 31250 baud using the MIDI Library.
- The damper potentiometer body spans Ring–Sleeve, so Ring baseline shifts with
  damper position. The decode logic for middle/left may need to detect a
  relative change from a rolling Ring baseline rather than fixed thresholds.
