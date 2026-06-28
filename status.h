// status.h
// NeoPixel status indicators for normal (non-calibration) operation.
// Call updateStatusLed() from loop() on every iteration.
//
//   Solid red   — no pedal detected
//   Solid green — pedal connected and ready

#pragma once

#include <Adafruit_NeoPixel.h>
#include "colors.h"
#include "debug.h"

// Two-threshold hysteresis for pedal connection detection.
// Tip reads ~3965 (connected+released), ~1813 (connected+pressed), ~1666 (unplugged).
// Single-threshold detection fails because pressing the damper brings Tip below any
// threshold that would also distinguish it from unplugged (~1666 vs ~1813, gap=147).
//
// HI threshold: Tip must rise above this to go connected (only possible when released).
// LO threshold: Tip must drop below this to go disconnected (1666 < 1740 ≤ 1813).
#define STATUS_PEDAL_THRESHOLD_HI  2500
#define STATUS_PEDAL_THRESHOLD_LO  1740

// Require the new state to hold this long before treating it as a real
// connect/disconnect event.
#define STATUS_PEDAL_DEBOUNCE_MS  500

// Hold red for this long at boot before starting pedal detection.
// Prevents false green from ADC transients during hardware initialization
// (USB host init, Sleeve 3.3V capacitive coupling, ADC first-sample noise).
#define STATUS_PEDAL_STARTUP_MS  1000

// Updates the NeoPixel to reflect current system state.
// Only calls pixel.show() when the state actually changes.
inline void updateStatusLed(Adafruit_NeoPixel& px, int tipRaw) {
  static uint32_t      lastColor        = 0xFFFFFFFF;
  static bool          lastPedalPresent = false;
  static bool          candidate        = false;
  static unsigned long candAt           = 0;
  static bool          started          = false;

  // During the startup grace period: show red and don't run the debounce.
  // This absorbs any transients that occur while hardware is initializing.
  if (!started) {
    if (millis() < STATUS_PEDAL_STARTUP_MS) {
      if (lastColor != PIXEL_RED) {
        px.setPixelColor(0, PIXEL_RED);
        px.show();
        lastColor = PIXEL_RED;
      }
      return;
    }
    // Grace period over — prime the debounce from the settled reading.
    started   = true;
    candidate = (tipRaw >= STATUS_PEDAL_THRESHOLD_HI);
    candAt    = millis();
  }

  // Hysteresis: which threshold applies depends on confirmed state.
  //   Not present → only go present if Tip rises above HI (pedal released)
  //   Present     → only go absent  if Tip drops below LO (unplugged, not just pressed)
  bool raw = lastPedalPresent
    ? (tipRaw >= STATUS_PEDAL_THRESHOLD_LO)
    : (tipRaw >= STATUS_PEDAL_THRESHOLD_HI);

  if (raw != candidate) {
    candidate = raw;
    candAt    = millis();
  }

  bool pedalPresent = lastPedalPresent;
  if (millis() - candAt >= STATUS_PEDAL_DEBOUNCE_MS && candidate != lastPedalPresent) {
    pedalPresent     = candidate;
    lastPedalPresent = pedalPresent;
    DEBUG_PRINTLN(pedalPresent ? "Pedal: connected" : "Pedal: disconnected");
  }

  uint32_t color = pedalPresent ? PIXEL_GREEN : PIXEL_RED;
  if (color != lastColor) {
    px.setPixelColor(0, color);
    px.show();
    lastColor = color;
  }
}
