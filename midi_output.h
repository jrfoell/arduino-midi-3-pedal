// midi_output.h
// Dual MIDI output: hardware DIN (MIDI FeatherWing via Serial1) and
// USB host MIDI (USB Host FeatherWing via MAX3421E + TinyUSB).
//
// Exposes:
//   initMidi()           — call once in setup()
//   usbMidiTask()        — call every loop() to service the USB host stack
//   sendCC(ch, cc, val)  — sends to both outputs simultaneously
//   usbMidiConnected     — true when a USB MIDI device is mounted

#pragma once

#include <SPI.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>

// ─── Pin assignments (USB Host FeatherWing / MAX3421E) ────────────────────────
#define PIN_USB_CS   10   // Chip select
#define PIN_USB_INT   9   // Interrupt

// ─── Hardware DIN MIDI (Serial1 via MIDI FeatherWing) ────────────────────────
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, hwMidi);

// ─── USB host (MAX3421E) ─────────────────────────────────────────────────────
// SPI bus, CS, and INT are passed to the constructor; begin() takes only the
// root hub port number (1 for MAX3421E).
static Adafruit_USBH_Host USBHost(&SPI, PIN_USB_CS, PIN_USB_INT);

// Interface index assigned by TinyUSB when a MIDI device mounts.
// Used in all tuh_midi_* API calls. 255 = no device present.
static uint8_t _usbMidiIdx = 255;

// Read by the main loop and passed to updateStatusLed().
static bool usbMidiConnected = false;

// ─── Init / task ──────────────────────────────────────────────────────────────

static void initMidi() {
  hwMidi.begin(MIDI_CHANNEL_OMNI);
  hwMidi.turnThruOff();
  USBHost.begin(1);  // 1 = root hub port for MAX3421E
}

static void usbMidiTask() {
  USBHost.task();
}

// ─── Send ─────────────────────────────────────────────────────────────────────

// Sends a MIDI CC on both outputs simultaneously.
// USB send is skipped if no device is currently connected.
static void sendCC(uint8_t channel, uint8_t cc, uint8_t value) {
  hwMidi.sendControlChange(cc, value, channel);

  if (usbMidiConnected) {
    uint8_t buf[3] = { (uint8_t)(0xB0 | (channel - 1)), cc, value };
    tuh_midi_stream_write(_usbMidiIdx, 0, buf, sizeof(buf));
    tuh_midi_write_flush(_usbMidiIdx);
  }
}

// ─── TinyUSB MIDI host callbacks ──────────────────────────────────────────────
// TinyUSB calls these as C functions; extern "C" is required in a C++ file.
// Signatures must match midi_host.h exactly — the API changed to idx + struct.

extern "C" {
  void tuh_midi_mount_cb(uint8_t idx, const tuh_midi_mount_cb_t *mount_cb_data) {
    (void)mount_cb_data;
    _usbMidiIdx      = idx;
    usbMidiConnected = true;
  }

  void tuh_midi_umount_cb(uint8_t idx) {
    (void)idx;
    _usbMidiIdx      = 255;
    usbMidiConnected = false;
  }
}
