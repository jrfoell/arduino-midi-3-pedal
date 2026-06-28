// midi_output.h
// MIDI output via hardware DIN (MIDI FeatherWing / Serial1) and
// USB host (USB Host FeatherWing / MAX3421E + TinyUSB).
//
// Exposes:
//   initMidi()         — call once in setup(), after Serial.begin()
//   usbMidiTask()      — call every loop(); services host stack + reads MIDI
//   sendCC(ch, cc, v)  — sends to all enabled outputs
//   usbMidiConnected   — true when a USB MIDI device is mounted

#pragma once

#include <SPI.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include "debug.h"

// ─── Hardware DIN MIDI (Serial1 via MIDI FeatherWing) ────────────────────────
// Requires HARDWARE_MIDI defined in the .ino (before all includes).
#ifdef HARDWARE_MIDI
  MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, hwMidi);
  #define MIDI_BEGIN()    hwMidi.begin(MIDI_CHANNEL_OMNI);
  #define MIDI_THRU_OFF() hwMidi.turnThruOff();
#else
  #define MIDI_BEGIN()    ((void)0)
  #define MIDI_THRU_OFF() ((void)0)
#endif

// ─── USB host (MAX3421E) ─────────────────────────────────────────────────────
// Requires HARDWARE_USB defined in the .ino (before all includes).
#ifdef HARDWARE_USB
  #define USB_HOST_BEGIN() USBHost.begin(1);

  #define PIN_USB_CS   10   // Chip select
  #define PIN_USB_INT   9   // Interrupt

  static Adafruit_USBH_Host USBHost(&SPI, PIN_USB_CS, PIN_USB_INT);

  typedef struct {
    tusb_desc_device_t desc_device;
    uint16_t manufacturer[32];
    uint16_t product[48];
    uint16_t serial[16];
    bool mounted;
  } dev_info_t;

  // CFG_TUH_DEVICE_MAX is defined by tusb_config header
  dev_info_t dev_info[CFG_TUH_DEVICE_MAX] = { 0 };

  // Interface index set by tuh_midi_mount_cb; used in all tuh_midi_* calls.
  static uint8_t _usbMidiIdx = 255;

  // True when a USB MIDI device is mounted and ready.
  static bool usbMidiConnected = false;

#else
  #define USB_HOST_BEGIN(baud)       ((void)0)
  // No USB host hardware — always report disconnected.
  static bool usbMidiConnected = false;
#endif

// ─── Init / task ──────────────────────────────────────────────────────────────

// Must be called after Serial.begin() — see Adafruit device_info_max3421e.
static void initMidi() {
  MIDI_BEGIN();
  MIDI_THRU_OFF();

  USB_HOST_BEGIN();
}


// ─── Send ─────────────────────────────────────────────────────────────────────

static void sendCC(uint8_t channel, uint8_t cc, uint8_t value) {
  #ifdef HARDWARE_MIDI
    hwMidi.sendControlChange(cc, value, channel);
  #endif

  #ifdef HARDWARE_USB
    if (usbMidiConnected) {
      uint8_t buf[3] = { (uint8_t)(0xB0 | (channel - 1)), cc, value };
      tuh_midi_stream_write(_usbMidiIdx, 0, buf, sizeof(buf));
      tuh_midi_write_flush(_usbMidiIdx);
    }
  #endif
}


#ifdef HARDWARE_USB

  // Call every loop(). Services the USB host stack and drains incoming MIDI.
  static void usbMidiTask() {
    static int activeNotes = 0;

    USBHost.task();

    if (!usbMidiConnected) return;

    // Drain all available MIDI packets from the device.
    // USB MIDI packets are 4 bytes: [header, status, data1, data2]
    uint8_t packet[4];
    while (tuh_midi_packet_read(_usbMidiIdx, packet)) {
      uint8_t status = packet[1];
      uint8_t type   = status & 0xF0;
      uint8_t ch     = (status & 0x0F) + 1;  // convert to 1-based

      if (type == 0x90 && packet[3] > 0) {   // NoteOn (velocity 0 = NoteOff)
        if (activeNotes == 0) digitalWrite(LED_BUILTIN, LOW);
        activeNotes++;
        DEBUG_PRINT("NoteOn   ch="); DEBUG_PRINT(ch);
        DEBUG_PRINT(" note=");       DEBUG_PRINT(packet[2]);
        DEBUG_PRINT(" vel=");        DEBUG_PRINTLN(packet[3]);
      } else if (type == 0x80 || (type == 0x90 && packet[3] == 0)) {
        if (activeNotes > 0) activeNotes--;
        if (activeNotes == 0) digitalWrite(LED_BUILTIN, HIGH);
        DEBUG_PRINT("NoteOff  ch="); DEBUG_PRINT(ch);
        DEBUG_PRINT(" note=");       DEBUG_PRINTLN(packet[2]);
      }
    }
  }

  // ─── USB host callbacks ───────────────────────────────────────────────────────
  extern "C" {
    // Generic — fires for any USB device class
    void tuh_mount_cb(uint8_t daddr) {
      DEBUG_PRINT("USB: device mounted  addr=");
      DEBUG_PRINTLN(daddr);
    }

    void tuh_umount_cb(uint8_t daddr) {
      DEBUG_PRINT("USB: device removed  addr=");
      DEBUG_PRINTLN(daddr);
    }

    // MIDI class — fires once the MIDI host driver has set up the device
    void tuh_midi_mount_cb(uint8_t idx, const tuh_midi_mount_cb_t *data) {
      _usbMidiIdx      = idx;
      usbMidiConnected = true;
      DEBUG_PRINT("USB MIDI: device ready  idx="); DEBUG_PRINT(idx);
      DEBUG_PRINT(" rx_cables=");                  DEBUG_PRINT(data->rx_cable_count);
      DEBUG_PRINT(" tx_cables=");                  DEBUG_PRINTLN(data->tx_cable_count);
    }

    void tuh_midi_umount_cb(uint8_t idx) {
      (void)idx;
      _usbMidiIdx      = 255;
      usbMidiConnected = false;
      DEBUG_PRINTLN("USB MIDI: device removed");
    }
  }
#else
  static void usbMidiTask() {
    // No USB host hardware — do nothing.
  }
#endif
