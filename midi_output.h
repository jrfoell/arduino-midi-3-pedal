// midi_output.h
// MIDI output via hardware DIN (MIDI FeatherWing / Serial1) and
// USB host (USB Host FeatherWing / MAX3421E + TinyUSB).
//
// USB host currently detects device mount/unmount only.
// MIDI send over USB host is TODO once detection is confirmed working.
//
// Exposes:
//   initMidi()         — call once in setup(), after Serial.begin()
//   usbMidiTask()      — call every loop() to service the USB host stack
//   sendCC(ch, cc, v)  — sends to all enabled outputs
//   usbMidiConnected   — true when any USB device is mounted on the host port

#pragma once

#include <SPI.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include "debug.h"

// ─── Pin assignments (USB Host FeatherWing / MAX3421E) ────────────────────────
#define PIN_USB_CS   10   // Chip select
#define PIN_USB_INT   9   // Interrupt

// ─── Hardware DIN MIDI (Serial1 via MIDI FeatherWing) ────────────────────────
// Requires HARDWARE_MIDI defined in the .ino (before all includes).
#ifdef HARDWARE_MIDI
  MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, hwMidi);
#endif

// ─── USB host (MAX3421E) ─────────────────────────────────────────────────────
static Adafruit_USBH_Host USBHost(&SPI, PIN_USB_CS, PIN_USB_INT);

// True when any USB device is mounted on the host port.
// Set/cleared by tuh_mount_cb / tuh_umount_cb.
static bool usbMidiConnected = false;

// ─── Init / task ──────────────────────────────────────────────────────────────

// Call after Serial.begin() — matches Adafruit device_info_max3421e example.
static void initMidi() {
  #ifdef HARDWARE_MIDI
    hwMidi.begin(MIDI_CHANNEL_OMNI);
    hwMidi.turnThruOff();
    DEBUG_PRINTLN("Hardware MIDI: initialized on Serial1");
  #endif

  USBHost.begin(1);
}

static void usbMidiTask() {
  USBHost.task();
}

// ─── Send ─────────────────────────────────────────────────────────────────────

// Sends a MIDI CC on all enabled outputs.
static void sendCC(uint8_t channel, uint8_t cc, uint8_t value) {
  #ifdef HARDWARE_MIDI
    hwMidi.sendControlChange(cc, value, channel);
  #endif

  // USB MIDI send — TODO once host device detection is confirmed
  (void)channel; (void)cc; (void)value;
}

// ─── USB host callbacks ───────────────────────────────────────────────────────
// Generic callbacks — fire for any USB device class, not just MIDI.
// Using these rather than tuh_midi_mount_cb to validate host detection first.

extern "C" {
  void tuh_mount_cb(uint8_t daddr) {
    usbMidiConnected = true;
    DEBUG_PRINT("USB: device mounted  addr=");
    DEBUG_PRINTLN(daddr);
  }

  void tuh_umount_cb(uint8_t daddr) {
    usbMidiConnected = false;
    DEBUG_PRINT("USB: device removed  addr=");
    DEBUG_PRINTLN(daddr);
  }
}
