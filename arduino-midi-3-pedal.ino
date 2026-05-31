// arduino-midi-3-pedal.ino
// Adafruit Feather M4 Express + MIDI FeatherWing
// Reads a triple piano pedal unit (TRS) and sends MIDI CC messages.

#include <Adafruit_NeoPixel.h>
#include "calibration.h"

// ─── Pin assignments ──────────────────────────────────────────────────────────
#define PIN_PIXEL         8   // NeoPixel data  (Feather M4: PIN_NEOPIXEL = 8)
#define PIN_PIXEL_POWER   2   // NeoPixel power (Feather M4: NEOPIXEL_POWER = 2)

// ─── MIDI ────────────────────────────────────────────────────────────────────
#define MIDI_CHANNEL      1
#define CC_DAMPER        64   // Right pedal  — Sustain / Damper
#define CC_SOSTENUTO     66   // Middle pedal — Sostenuto
#define CC_SOFT          67   // Left pedal   — Soft / Una Corda

// ─── NeoPixel ────────────────────────────────────────────────────────────────
#define PIXEL_BRIGHTNESS      40   // 0–255

// ─── Globals ─────────────────────────────────────────────────────────────────
Adafruit_NeoPixel pixel(1, PIN_PIXEL, NEO_GRB + NEO_KHZ800);
CalibrationData calData;

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
  analogReadResolution(12);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  pinMode(PIN_PIXEL_POWER, OUTPUT);
  digitalWrite(PIN_PIXEL_POWER, HIGH);
  pixel.begin();
  pixel.setBrightness(PIXEL_BRIGHTNESS);
  pixel.show();

  pinMode(PIN_TIP,  INPUT);
  pinMode(PIN_RING, INPUT);

  if (!loadCalibration(calData)) {
    calData = defaultCalibration();
  }

  // Enter calibration if the left or middle pedal (or both) is held at power-on
  if (pedalHeldAtBoot(calData)) {
    runCalibration(pixel, calData);
  }

  // MIDI will initialize here (Serial1 @ 31250 via MIDI FeatherWing)
}

// ─── Loop ────────────────────────────────────────────────────────────────────
void loop() {

  // int tipRaw  = analogRead(PIN_TIP);
  // int ringRaw = analogRead(PIN_RING);
  // Pedal decode and MIDI output will go here
}
