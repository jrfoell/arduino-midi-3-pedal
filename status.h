// status.h
// NeoPixel status indicators for normal (non-calibration) operation.
// Call updateStatusLed() from loop() on every iteration.
//
// Pedal presence gates everything — red until a pedal is attached,
// regardless of MIDI connection state:
//   Solid red   — no pedal detected (overrides MIDI state)
//   Off         — pedal connected, no USB MIDI device yet
//   Solid green — pedal connected AND USB MIDI device connected

#pragma once

#include <Adafruit_NeoPixel.h>
#include "debug.h"

// Tip ADC reading below this value means no pedal is plugged in.
// With no pedal: Tip reads ~0 (10kΩ pull-down, nothing driving it).
// With pedal connected and damper at rest: Tip reads ~3950.
#define STATUS_PEDAL_THRESHOLD  500

// Updates the NeoPixel to reflect current system state.
// Only calls pixel.show() when the state actually changes.
// usbMidiConnected: pass the mount-state flag from midi_output.h once implemented.
inline void updateStatusLed(Adafruit_NeoPixel& px, int tipRaw, bool usbMidiConnected) {
  static uint32_t lastColor        = 0xFFFFFFFF;
  static bool     lastPedalPresent = false;

  bool pedalPresent = (tipRaw >= STATUS_PEDAL_THRESHOLD);
  if (pedalPresent != lastPedalPresent) {
    DEBUG_PRINTLN(pedalPresent ? "Pedal: connected" : "Pedal: disconnected");
    lastPedalPresent = pedalPresent;
  }

  uint32_t color;
  if (!pedalPresent) {
    color = px.Color(200, 0, 0);  // Red: no pedal — MIDI state ignored
  } else if (usbMidiConnected) {
    color = px.Color(0, 200, 0);  // Green: pedal + MIDI both present
  } else {
    color = 0;                    // Off: pedal present, waiting for MIDI device
  }

  if (color != lastColor) {
    px.setPixelColor(0, color);
    px.show();
    lastColor = color;
  }
}
