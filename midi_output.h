// midi_output.h
// Dual MIDI output: hardware DIN (MIDI FeatherWing via Serial1) and
// USB MIDI device (Feather enumerates as a USB MIDI device on its native port).
//
// Exposes:
//   initMidi()           — call once in setup(), before Serial.begin()
//   usbMidiTask()        — call every loop() to service TinyUSB and read MIDI
//   sendCC(ch, cc, val)  — sends to both outputs simultaneously
//   usbMidiConnected     — true when a USB host has mounted this device

#pragma once

#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include "debug.h"

// ─── Hardware DIN MIDI (Serial1 via MIDI FeatherWing) ────────────────────────
// Requires HARDWARE_MIDI defined in the .ino (before all includes).
// Leave undefined when the MIDI FeatherWing is not physically attached.
#ifdef HARDWARE_MIDI
  MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, hwMidi);
#endif

// ─── USB MIDI device ──────────────────────────────────────────────────────────
// The Feather enumerates as a USB MIDI device on its native USB port.
// A computer or a keyboard with USB host capability connects to it.
static Adafruit_USBD_MIDI _usbMidi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, _usbMidi, usbMidi);

// True when a USB host has enumerated and mounted this device.
// Updated each loop by usbMidiTask(); read by updateStatusLed().
static bool usbMidiConnected = false;

// ─── Init ─────────────────────────────────────────────────────────────────────

static void initMidi() {
  // Required on cores without built-in TinyUSB support
  if (!TinyUSBDevice.isInitialized()) {
    TinyUSBDevice.begin(0);
  }

  _usbMidi.setStringDescriptor("Pedal Controller");

  // begin() also calls _usbMidi.begin()
  usbMidi.begin(MIDI_CHANNEL_OMNI);

  // Force re-enumeration if already mounted (e.g. after a warm reset)
  if (TinyUSBDevice.mounted()) {
    TinyUSBDevice.detach();
    delay(10);
    TinyUSBDevice.attach();
  }

  #ifdef HARDWARE_MIDI
    hwMidi.begin(MIDI_CHANNEL_OMNI);
    hwMidi.turnThruOff();
    DEBUG_PRINTLN("Hardware MIDI: initialized on Serial1");
  #endif
}

// ─── Task ─────────────────────────────────────────────────────────────────────

// Call every loop(). Services TinyUSB, tracks mount state, reads incoming MIDI.
static void usbMidiTask() {
  #ifdef TINYUSB_NEED_POLLING_TASK
    TinyUSBDevice.task();
  #endif

  bool mounted = TinyUSBDevice.mounted();

  if (mounted != usbMidiConnected) {
    usbMidiConnected = mounted;
    DEBUG_PRINTLN(mounted ? "USB MIDI: host connected" : "USB MIDI: host disconnected");
  }

  if (mounted) {
    usbMidi.read();
  }
}

// ─── Send ─────────────────────────────────────────────────────────────────────

// Sends a MIDI CC on all enabled outputs simultaneously.
static void sendCC(uint8_t channel, uint8_t cc, uint8_t value) {
  #ifdef HARDWARE_MIDI
    hwMidi.sendControlChange(cc, value, channel);
  #endif

  if (usbMidiConnected) {
    usbMidi.sendControlChange(cc, value, channel);
  }
}
