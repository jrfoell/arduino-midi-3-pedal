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

// Blink LED_BUILTIN n times — works before Serial is ready.
// Move this call through setup() to bisect a hang.
static inline void blinkLED(int n) {
  pinMode(LED_BUILTIN, OUTPUT);
  for (int i = 0; i < n; i++) {
    digitalWrite(LED_BUILTIN, HIGH); delay(150);
    digitalWrite(LED_BUILTIN, LOW);  delay(150);
  }
  delay(400);  // pause after sequence so bursts are visually distinct
}

// Non-blocking heartbeat — call at the top of loop() to confirm it is running.
// Toggles LED_BUILTIN every 500 ms; visible as a steady 1 Hz blink.
static inline void blinkHeartbeat() {
  static unsigned long last = 0;
  static bool state = false;
  unsigned long now = millis();
  if (now - last >= 500) {
    last  = now;
    state = !state;
    digitalWrite(LED_BUILTIN, state ? HIGH : LOW);
  }
}
