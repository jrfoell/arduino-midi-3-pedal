// arduino-midi-3-pedal.ino
// Adafruit Feather M4 Express + MIDI FeatherWing
// Reads a triple piano pedal unit (TRS) and sends MIDI CC messages.

// ─── Debug ───────────────────────────────────────────────────────────────────
// Uncomment to enable serial debug output. Must appear before all #includes.
// #define DEBUG_SERIAL

// ─── Hardware feature flags ──────────────────────────────────────────────────
// Uncomment when the MIDI FeatherWing is physically attached.
// #define HARDWARE_MIDI

#include "Adafruit_TinyUSB.h"
#include "SPI.h"
#include "debug.h"
// #include <Adafruit_NeoPixel.h>
// #include "midi_output.h"
// #include "calibration.h"
// #include "status.h"
// #include "pedal_decode.h"

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
// USB Host using MAX3421E: SPI, CS, INT
// Default CS and INT are pin 10, 9
Adafruit_USBH_Host USBHost(&SPI, 10, 9);

typedef struct {
  tusb_desc_device_t desc_device;
  uint16_t manufacturer[32];
  uint16_t product[48];
  uint16_t serial[16];
  bool mounted;
} dev_info_t;

// CFG_TUH_DEVICE_MAX is defined by tusb_config header
dev_info_t dev_info[CFG_TUH_DEVICE_MAX] = { 0 };

// Adafruit_NeoPixel pixel(1, PIN_PIXEL, NEO_GRB + NEO_KHZ800);
// CalibrationData calData;

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  Serial.begin(115200);

  // init host stack on controller (rhport) 1
  USBHost.begin(1);
  DEBUG_PRINTLN("Initialized USB Host stack");

  // DEBUG_PRINTLN("Turning on status LED");

  // blinkHeartbeat();

  // DEBUG_PRINTLN("Initializing MIDI");
  // initMidi();

  // DEBUG_PRINTLN("Setting analog read resolution to 12 bits (0–4095)");
  // analogReadResolution(12);

  // DEBUG_PRINTLN("Initializing NeoPixel");
  // pinMode(PIN_PIXEL_POWER, OUTPUT);
  // digitalWrite(PIN_PIXEL_POWER, HIGH);
  // pixel.begin();
  // pixel.setBrightness(PIXEL_BRIGHTNESS);
  // pixel.show();

  // pinMode(PIN_TIP,  INPUT);
  // pinMode(PIN_RING, INPUT);

  // DEBUG_PRINTLN("Checking for calibration");
  // if (!loadCalibration(calData)) {
  //   calData = defaultCalibration();
  // }

  // // Enter calibration if the left or middle pedal (or both) is held at power-on
  // if (pedalHeldAtBoot(calData, pixel)) {
  //   runCalibration(pixel, calData);
  // }
}

// ─── Loop ────────────────────────────────────────────────────────────────────
void loop() {
  USBHost.task();
  DEBUG_FLUSH();

  // blinkHeartbeat();
  // usbMidiTask();

  // int tipRaw  = analogRead(PIN_TIP);
  // int ringRaw = analogRead(PIN_RING);

  //updateStatusLed(pixel, tipRaw, usbMidiConnected);

  // Damper pedal (right / Tip)
  // int damperCC = updateDamper(tipRaw, calData);
  // if (damperCC >= 0) {
  //   sendCC(MIDI_CHANNEL, CC_DAMPER, (uint8_t)damperCC);
  // }
}

// Invoked when device is mounted (configured)
void tuh_mount_cb(uint8_t daddr) {
  Serial.printf("Device attached, address = %d\r\n", daddr);

  dev_info_t *dev = &dev_info[daddr - 1];
  dev->mounted = true;
}

/// Invoked when device is unmounted (bus reset/unplugged)
void tuh_umount_cb(uint8_t daddr) {
  Serial.printf("Device removed, address = %d\r\n", daddr);
  dev_info_t *dev = &dev_info[daddr - 1];
  dev->mounted = false;
}

