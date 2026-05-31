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

// Returns the new CC value (0–127) if it changed, or -1 if unchanged.
inline int updateDamper(int tipRaw, const CalibrationData& cal) {
  static int lastCC = -1;

  int cc = map(tipRaw, cal.tipRest, cal.tipPressed, 0, 127);
  cc = constrain(cc, 0, 127);

  if (cc == lastCC) return -1;

  if (cc > 0 && lastCC <= 0) DEBUG_PRINTLN("Damper: pressed");
  if (cc == 0 && lastCC > 0) DEBUG_PRINTLN("Damper: released");

  lastCC = cc;
  return cc;
}

// ─── Ring pedals (middle + left) ─────────────────────────────────────────────
// TODO: implement updateSostenuto() and updateSoft() using Ring ADC +
// calibration thresholds (threshNoneToMiddle, threshMiddleToLeft, threshLeftToBoth).
// Both are binary (on/off) with millis()-based debounce.
