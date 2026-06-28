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

// Tip ADC reading below this value means no pedal is plugged in.
// With no pedal: Tip reads ~0 (10kΩ pull-down, nothing driving it).
// With pedal connected and damper at rest: Tip reads ~3950.
#define STATUS_PEDAL_THRESHOLD  500

// A floating pin crosses the threshold rapidly; require the new state to hold
// this long before treating it as a real connect/disconnect event.
#define STATUS_PEDAL_DEBOUNCE_MS  500

// Updates the NeoPixel to reflect current system state.
// Only calls pixel.show() when the state actually changes.
inline void updateStatusLed(Adafruit_NeoPixel& px, int tipRaw) {
  static uint32_t lastColor        = 0xFFFFFFFF;
  static bool     lastPedalPresent = false;
  static bool     candidate        = false;
  static unsigned long candAt      = 0;

  bool raw = (tipRaw >= STATUS_PEDAL_THRESHOLD);
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
