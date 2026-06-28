# Wiring Schematic: Feather M4 Express → TRS Jack

## Components

| Ref | Part | Notes |
|---|---|---|
| U1 | Adafruit Feather M4 Express (ATSAMD51) | 3.3V logic, 12-bit ADC |
| J1 | 1/4" TRS panel-mount jack | Tip, Ring, Sleeve solder lugs |
| R1 | 330Ω | Series current-limiting resistor on Sleeve power rail (keep small — see Notes) |
| R2 | 10kΩ | Pull-down on Tip (A2) — also serves as R in the RC noise filter with C1 |
| R3 | 10kΩ | Pull-down on Ring (A3) — also serves as R in the RC noise filter with C2 |
| C1 | 100nF ceramic | Noise filter on Tip (A2) to GND — pairs with R2, fc ≈ 160 Hz |
| C2 | 100nF ceramic | Noise filter on Ring (A3) to GND — pairs with R3, fc ≈ 160 Hz |

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
  │                   │    │        │                │                   │
  │                   │   R2      C1 ← noise filter  │                   │
  │                   │   10kΩ   100nF               │                   │
  │                   │    │        │                │                   │
  │              GND  ├────┴────────┘                │                   │
  │                   │                              │                   │
  │               A3  ├──────────────────────────────┤ R  (Ring)         │
  │                   │    │        │                │                   │
  │                   │   R3      C2 ← noise filter  └───────────────────┘
  │                   │   10kΩ   100nF
  │                   │    │        │
  │              GND  ├────┴────────┘
  │                   │
  └───────────────────┘
```

---

## Signal roles

| TRS terminal | Feather pin | Passives | Role |
|---|---|---|---|
| S — Sleeve | 3V3 | R1 330Ω in series | 3.3V power rail into pedal unit |
| T — Tip    | A2  | R2 10kΩ to GND, C1 100nF to GND | Right pedal (damper) potentiometer wiper |
| R — Ring   | A3  | R3 10kΩ to GND, C2 100nF to GND | Middle + Left pedal resistor network |

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

**Tip (A2)** — The damper potentiometer wiper. Reads proportionally between 0 V (pedal fully released) and ~3.3 V (pedal fully pressed), enabling half-pedal detection.

---

## Notes

- R1 must be kept small. Because it sits in series with the internal pedal resistors and R3, a large R1 compresses all four voltage levels into a narrow band, making reliable decoding impossible. Its only job is fault protection (limiting current if the cable shorts). 330Ω or less is fine; anything above ~1kΩ starts squeezing the voltage windows noticeably.
- R2 and R3 should be matched; asymmetry shifts all thresholds.
- C1 and C2 form low-pass RC filters with R2 and R3 respectively (τ = 10kΩ × 100nF = 1 ms, fc ≈ 160 Hz). This attenuates ADC sampling noise and transient crosstalk between the Tip and Ring lines that occurs when the Ring pedal switches bounce. Place the capacitors physically close to the A2/A3 pins on the Feather, not at the TRS jack. Use ceramic capacitors (not electrolytic) — no polarity concerns and low ESR. R2 and R3 serve double duty as both the pull-down (detecting a disconnected pedal) and the R in the RC filter; no separate series resistors are needed.
- All resistor values should be confirmed against actual ADC calibration readings before encoding thresholds in the main sketch.
- The Feather M4 ADC is **12-bit** (0–4095). Voltage-to-ADC count: `count = voltage × 4095 / 3.3`.
