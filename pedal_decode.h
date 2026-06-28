// pedal_decode.h
// Decodes raw ADC readings into MIDI CC values using calibration data.
// Each update function tracks its own last-sent value and returns true
// only when the output has changed — call sendCC() on a true return.

#pragma once

#include "calibration.h"
#include "debug.h"

// ─── Damper (right pedal / Tip) ───────────────────────────────────────────────
// Potentiometer — supports half-pedaling. Uses a two-segment piecewise map:
//   first DAMPER_SUSTAIN_ON_PCT% of active travel → CC 0–63
//   remaining travel through DAMPER_FULL_PCT% → CC 64–127
// CC 64 (sustain on) therefore kicks in early; CC 127 is reached before the
// mechanical floor so it is reliably achieved in normal playing.
// Off (pedal released) sends CC 0.

// Dead zone applied before the damper mapping to absorb two noise sources:
//   1. ADC noise at rest (~150 counts observed)
//   2. Electrical crosstalk from Ring pedal switches (~200 counts on Tip)
// Increase if false presses persist; decrease if response feels sluggish.
#define ADC_DEAD_ZONE 250

// How often to print the live CC level while the damper pedal is held (ms).
#define DAMPER_DEBUG_PRINT_MS 500

// Hysteresis band above the press threshold — pedal must rise this many counts
// above pressThresh before a release is registered. Prevents oscillation when
// the pedal hovers near the threshold. Must be larger than the observed ADC
// swing at the boundary (~100 counts from the serial output).
#define DAMPER_HYSTERESIS 150

// Physical travel percentage at which CC reaches 127. Readings past this point
// are clamped to 127. Values below 100 ensure full sustain is reliably achieved
// before the mechanical floor.
#define DAMPER_FULL_PCT 97

// Active travel percentage at which CC 64 (sustain on) is first reached.
// Lower values front-load the sustain region so it triggers on a lighter press.
#define DAMPER_SUSTAIN_ON_PCT 33

// Returns the new CC value (0–127) if it changed, or -1 if unchanged.
inline int updateDamper(int tipRaw, const CalibrationData& cal) {
  static int  lastCC = -1;
  static bool active = false;

  // Tip falls when pressed: go active below pressThresh, release only above releaseThresh.
  int pressThresh   = cal.tipRest - ADC_DEAD_ZONE;
  int releaseThresh = pressThresh + DAMPER_HYSTERESIS;

  if (!active && tipRaw < pressThresh) {
    active = true;
  } else if (active && tipRaw >= releaseThresh) {
    active = false;
  }

  int cc = 0;
  if (active) {
    // tipAtFull: raw value at DAMPER_FULL_PCT% of physical travel (CC 127 ceiling).
    // sustainOnRaw: raw value at DAMPER_SUSTAIN_ON_PCT% of active travel (CC 64 breakpoint).
    // Tip falls as the pedal is pressed, so both targets are below pressThresh.
    int tipAtFull    = cal.tipRest - (int)((long)(cal.tipRest - cal.tipPressed) * DAMPER_FULL_PCT / 100);
    int activeRange  = pressThresh - tipAtFull;
    int sustainOnRaw = pressThresh - (int)((long)activeRange * DAMPER_SUSTAIN_ON_PCT / 100);
    if (tipRaw >= sustainOnRaw) {
      // First segment: light press — CC 0–63
      cc = map(tipRaw, pressThresh, sustainOnRaw, 0, 63);
      cc = constrain(cc, 0, 63);
    } else {
      // Second segment: sustain active — CC 64–127
      cc = map(tipRaw, sustainOnRaw, tipAtFull, 64, 127);
      cc = constrain(cc, 64, 127);
    }
  }

  // Continuous status print while pedal is held — shows live CC level (64–127).
  if (active) {
    static unsigned long lastPrint = 0;
    unsigned long now = millis();
    if (now - lastPrint >= DAMPER_DEBUG_PRINT_MS) {
      lastPrint = now;
      DEBUG_PRINT("Damper: held     (raw="); DEBUG_PRINT(tipRaw);
      DEBUG_PRINT(" cc="); DEBUG_PRINT(cc); DEBUG_PRINTLN(")");
    }
  }

  if (cc == lastCC) return -1;

  bool wasOn = (lastCC > 0);
  bool isOn  = (cc    > 0);
  lastCC = cc;

  if (isOn && !wasOn) {
    DEBUG_PRINT("Damper: pressed  (raw="); DEBUG_PRINT(tipRaw);
    DEBUG_PRINT(" cc="); DEBUG_PRINT(cc); DEBUG_PRINTLN(")");
  } else if (!isOn && wasOn) {
    DEBUG_PRINT("Damper: released (raw="); DEBUG_PRINT(tipRaw);
    DEBUG_PRINTLN(")");
  }

  return cc;
}

// ─── Ring pedals (middle + left) ─────────────────────────────────────────────
// Both pedals share the Ring conductor (A3) via a resistor network.
// Different voltage levels encode which pedal(s) are pressed.

#define RING_DEBOUNCE_MS 30   // Reject transitions shorter than this (ms)

// Returns zone code for the current Ring reading:
//   0 = none, 1 = middle only, 2 = left only, 3 = middle + left
static inline int decodeRingZone(int ringRaw, const CalibrationData& cal) {
  if (ringRaw >= threshLeftToBoth(cal))   return 3;
  if (ringRaw >= threshMiddleToLeft(cal)) return 2;
  if (ringRaw >= threshNoneToMiddle(cal)) return 1;
  return 0;
}

// Returns new CC value (0 or 127) if sostenuto (middle pedal) state changed, -1 if unchanged.
inline int updateSostenuto(int ringRaw, const CalibrationData& cal) {
  static int           lastCC    = -1;
  static bool          confirmed = false;
  static bool          candidate = false;
  static unsigned long candAt    = 0;

  int  zone    = decodeRingZone(ringRaw, cal);
  bool current = (zone == 1 || zone == 3);  // middle-only or both

  if (current != candidate) {
    candidate = current;
    candAt    = millis();
  }
  if (millis() - candAt < RING_DEBOUNCE_MS) return -1;
  if (candidate == confirmed)               return -1;

  confirmed = candidate;
  int cc    = confirmed ? 127 : 0;

  DEBUG_PRINT("Sostenuto: "); DEBUG_PRINT(confirmed ? "pressed " : "released");
  DEBUG_PRINT(" (raw=");     DEBUG_PRINT(ringRaw);
  DEBUG_PRINT(" zone=");     DEBUG_PRINT(zone);
  DEBUG_PRINT(" cc=");       DEBUG_PRINT(cc); DEBUG_PRINTLN(")");

  if (cc == lastCC) return -1;
  lastCC = cc;
  return cc;
}

// Returns new CC value (0 or 127) if soft (left pedal) state changed, -1 if unchanged.
inline int updateSoft(int ringRaw, const CalibrationData& cal) {
  static int           lastCC    = -1;
  static bool          confirmed = false;
  static bool          candidate = false;
  static unsigned long candAt    = 0;

  int  zone    = decodeRingZone(ringRaw, cal);
  bool current = (zone == 2 || zone == 3);  // left-only or both

  if (current != candidate) {
    candidate = current;
    candAt    = millis();
  }
  if (millis() - candAt < RING_DEBOUNCE_MS) return -1;
  if (candidate == confirmed)               return -1;

  confirmed = candidate;
  int cc    = confirmed ? 127 : 0;

  DEBUG_PRINT("Soft:      "); DEBUG_PRINT(confirmed ? "pressed " : "released");
  DEBUG_PRINT(" (raw=");     DEBUG_PRINT(ringRaw);
  DEBUG_PRINT(" zone=");     DEBUG_PRINT(zone);
  DEBUG_PRINT(" cc=");       DEBUG_PRINT(cc); DEBUG_PRINTLN(")");

  if (cc == lastCC) return -1;
  lastCC = cc;
  return cc;
}
