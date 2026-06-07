// pedal_decode.h
// Decodes raw ADC readings into MIDI CC values using calibration data.
// Each update function tracks its own last-sent value and returns true
// only when the output has changed — call sendCC() on a true return.

#pragma once

#include "calibration.h"
#include "debug.h"

// ─── Damper (right pedal / Tip) ───────────────────────────────────────────────
// Potentiometer — supports half-pedaling. Tip is high at rest and falls when
// pressed, so the map() arguments are ordered tipRest→0, tipPressed→127.

// ADC noise floor across all three pedals (observed ~150 counts at rest).
// Applied as a dead zone before any threshold or mapping calculation.
// Increase if false presses persist; decrease if response feels sluggish.
#define ADC_DEAD_ZONE 200

// Returns the new CC value (0–127) if it changed, or -1 if unchanged.
inline int updateDamper(int tipRaw, const CalibrationData& cal) {
  static int  lastCC     = -1;
  static bool wasPressed = false;

  // Clamp readings within the dead zone to tipRest so they map to CC 0.
  // This absorbs resting voltage noise without needing a CC-level threshold.
  int tipClamped = (tipRaw >= (cal.tipRest - ADC_DEAD_ZONE)) ? cal.tipRest : tipRaw;
  int cc = map(tipClamped, cal.tipRest, cal.tipPressed, 0, 127);
  cc = constrain(cc, 0, 127);

  bool isPressed = (cc > 0);

  if (isPressed && !wasPressed) {
    wasPressed = true;
    // DEBUG_PRINT("Damper: pressed  (raw="); DEBUG_PRINT(tipRaw);
    // DEBUG_PRINT(" clamped="); DEBUG_PRINT(tipClamped);
    // DEBUG_PRINT(" cc="); DEBUG_PRINT(cc); DEBUG_PRINTLN(")");
  } else if (!isPressed && wasPressed) {
    wasPressed = false;
    // DEBUG_PRINT("Damper: released (raw="); DEBUG_PRINT(tipRaw);
    // DEBUG_PRINT(" clamped="); DEBUG_PRINT(tipClamped);
    // DEBUG_PRINT(" cc="); DEBUG_PRINT(cc); DEBUG_PRINTLN(")");
  }

  if (cc == lastCC) return -1;
  lastCC = cc;
  return cc;
}

// ─── Ring pedals (middle + left) ─────────────────────────────────────────────
// TODO: implement updateSostenuto() and updateSoft() using Ring ADC +
// calibration thresholds (threshNoneToMiddle, threshMiddleToLeft, threshLeftToBoth).
// Both are binary (on/off) with millis()-based debounce.
