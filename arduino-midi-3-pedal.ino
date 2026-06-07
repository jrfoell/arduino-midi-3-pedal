// arduino-midi-3-pedal.ino
// Adafruit Feather M4 Express + MIDI FeatherWing
// Reads a triple piano pedal unit (TRS) and sends MIDI CC messages.

// ─── Debug ───────────────────────────────────────────────────────────────────
// Uncomment to enable serial debug output. Must appear before all #includes.
#define DEBUG_SERIAL

// ─── Hardware feature flags ──────────────────────────────────────────────────
// Uncomment when the MIDI FeatherWing is physically attached.
#define HARDWARE_MIDI
// Uncomment when the USB Host FeatherWing (MAX3421E) is physically attached.
// #define HARDWARE_USB

#include <Adafruit_NeoPixel.h>
#include <Adafruit_TinyUSB.h>
#include <SPI.h>
#include "debug.h"
#include "midi_output.h"
#include "calibration.h"
#include "status.h"
#include "pedal_decode.h"

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
Adafruit_NeoPixel pixel(1, PIN_PIXEL, NEO_GRB + NEO_KHZ800);

CalibrationData calData;

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
  DEBUG_BEGIN(115200);

  DEBUG_PRINTLN("Turning on power LED");
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  blinkHeartbeat();

  DEBUG_PRINTLN("Initializing MIDI");
  initMidi();

  DEBUG_PRINTLN("Setting analog read inputs");
  analogReadResolution(12);
  pinMode(PIN_TIP,  INPUT);
  pinMode(PIN_RING, INPUT);

  DEBUG_PRINTLN("Initializing NeoPixel");
  pinMode(PIN_PIXEL_POWER, OUTPUT);
  digitalWrite(PIN_PIXEL_POWER, HIGH);
  pixel.begin();
  pixel.setBrightness(PIXEL_BRIGHTNESS);
  pixel.show();

  DEBUG_PRINTLN("Checking for calibration");
  if (!loadCalibration(calData)) {
    calData = defaultCalibration();
  }

  // Enter calibration if the left or middle pedal (or both) is held at power-on
  if (pedalHeldAtBoot(calData, pixel)) {
    runCalibration(pixel, calData);
  }
}

// ─── Loop ────────────────────────────────────────────────────────────────────
void loop() {
  blinkHeartbeat();

  int tipRaw  = analogRead(PIN_TIP);
  int ringRaw = analogRead(PIN_RING);

  updateStatusLed(pixel, tipRaw, usbMidiConnected);

  // Damper pedal (right / Tip)
  int damperCC = updateDamper(tipRaw, calData);
  if (damperCC >= 0) {
    sendCC(MIDI_CHANNEL, CC_DAMPER, (uint8_t)damperCC);
  }

  usbMidiTask();

  DEBUG_FLUSH();
}

