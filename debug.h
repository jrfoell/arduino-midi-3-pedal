// debug.h
// Conditional serial debug output.
// Define DEBUG_SERIAL in arduino-midi-3-pedal.ino BEFORE all #includes to enable.
// When disabled, all macros compile to nothing with zero runtime cost.

#pragma once

#ifdef DEBUG_SERIAL
  #define DEBUG_BEGIN(baud)       Serial.begin(baud)
  #define DEBUG_PRINT(x)          Serial.print(x)
  #define DEBUG_PRINTLN(x)        Serial.println(x)
  #define DEBUG_PRINT_VAL(lbl, v) do { Serial.print(lbl); Serial.print(": "); Serial.println(v); } while (0)
  #define DEBUG_FLUSH()           Serial.flush()
#else
  #define DEBUG_BEGIN(baud)       ((void)0)
  #define DEBUG_PRINT(x)          ((void)0)
  #define DEBUG_PRINTLN(x)        ((void)0)
  #define DEBUG_PRINT_VAL(lbl, v) ((void)0)
  #define DEBUG_FLUSH()           ((void)0)
#endif

// Non-blocking blink — call at the top of loop() to confirm it is running.
// Toggles LED_BUILTIN every 500 ms; visible as a steady 1 Hz blink.
static inline void blinkLED() {
  static unsigned long last = 0;
  static bool state = false;
  unsigned long now = millis();
  if (now - last >= 500) {
    last  = now;
    state = !state;
    digitalWrite(LED_BUILTIN, state ? HIGH : LOW);
  }
}
