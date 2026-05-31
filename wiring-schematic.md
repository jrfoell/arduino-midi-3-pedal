# Wiring Schematic: Feather M4 Express → TRS Jack

## Components

| Ref | Part | Notes |
|---|---|---|
| U1 | Adafruit Feather M4 Express (ATSAMD51) | 3.3V logic, 12-bit ADC |
| J1 | 1/4" TRS panel-mount jack | Tip, Ring, Sleeve solder lugs |
| R1 | 330Ω | Series current-limiting resistor on Sleeve power rail (keep small — see Notes) |
| R2 | 10kΩ | Pull-down on Tip (A2) signal |
| R3 | 10kΩ | Pull-down on Ring (A3) signal |

---

## Schematic

```
  Feather M4 Express                                 TRS Jack
  ┌───────────────────┐                              ┌───────────────────┐
  │                   │       R1 (fault protection)  │                   │
  │              3V3  ├──────/\/\────────────────────┤ S  (Sleeve)  3.3V │
  │                   │      330Ω  keep small!       │                   │
  │                   │                              │                   │
  │               A2  ├──────────────────────────────┤ T  (Tip)          │
  │                   │    │                         │                   │
  │                   │   R2  ← pull-down            │                   │
  │                   │   10kΩ                       │                   │
  │                   │    │                         │                   │
  │              GND  ├────┴────┐                    │                   │
  │                   │         │                    │                   │
  │               A3  ├─────────┼────────────────────┤ R  (Ring)         │
  │                   │    │    │                    │                   │
  │                   │   R3  ← pull-down            └───────────────────┘
  │                   │   10kΩ  │
  │                   │    │    │
  │              GND  ├────┴────┘
  │                   │
  └───────────────────┘
```

---

## Signal roles

| TRS terminal | Feather pin | Resistor | Role |
|---|---|---|---|
| S — Sleeve | 3V3 | R1 330Ω in series | 3.3V power rail into pedal unit |
| T — Tip    | A2  | R2 10kΩ to GND    | Right pedal (damper) potentiometer wiper |
| R — Ring   | A3  | R3 10kΩ to GND    | Middle + Left pedal resistor network |

---

## How the voltage dividers work

**Sleeve (S)** enters the pedal unit at 3.3V (through R1, which limits current).

**Ring (A3)** — When no switch pedal is pressed, A3 is pulled to GND through R3 (reads ~0). When a switch closes, the internal pedal resistor connects Ring to Sleeve (3.3V), forming a divider with R3:

```
  3.3V (Sleeve)
       │
  [pedal resistor]  ← varies by pedal/combination
       │
      A3 ──── ADC read
       │
     [R3: 10kΩ]
       │
      GND
```

| Pedal state | Internal resistance | Approx. A3 voltage | Approx. ADC count (0–4095) |
|---|---|---|---|
| None pressed | ∞ (open) | 0 V | 0 |
| Middle only | 10kΩ | ~1.62 V | ~2014 |
| Left only | 5.6kΩ | ~2.07 V | ~2571 |
| Middle + Left | 3.6kΩ (parallel) | ~2.37 V | ~2940 |

*(Calculated with R1 = 330Ω, R3 = 10kΩ. Formula: V = 3.3 × R3 / (R1 + R\_internal + R3))*

> Note: the damper potentiometer body also spans Ring–Sleeve, so A3's baseline shifts continuously with damper pedal position. Threshold decoding for Middle and Left must detect a voltage change relative to the current baseline, not fixed absolute values.

**Tip (A2)** — The damper potentiometer wiper. Reads proportionally between 0 V (pedal fully released) and ~3.3 V (pedal fully pressed), enabling half-pedal detection.

---

## Notes

- R1 must be kept small. Because it sits in series with the internal pedal resistors and R3, a large R1 compresses all four voltage levels into a narrow band, making reliable decoding impossible. Its only job is fault protection (limiting current if the cable shorts). 330Ω or less is fine; anything above ~1kΩ starts squeezing the voltage windows noticeably.
- R2 and R3 should be matched; asymmetry shifts all thresholds.
- All resistor values should be confirmed against actual ADC calibration readings before encoding thresholds in the main sketch.
- The Feather M4 ADC is **12-bit** (0–4095). Voltage-to-ADC count: `count = voltage × 4095 / 3.3`.
