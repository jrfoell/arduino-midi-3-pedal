// debug.h
// Conditional serial debug output.
// Define DEBUG_SERIAL in arduino-midi-3-pedal.ino BEFORE all #includes to enable.
// When disabled, all macros compile to nothing with zero runtime cost.

#pragma once

#ifdef DEBUG_SERIAL
  #define DEBUG_PRINT(x)          Serial.print(x)
  #define DEBUG_PRINTLN(x)        Serial.println(x)
  #define DEBUG_PRINT_VAL(lbl, v) do { Serial.print(lbl); Serial.print(": "); Serial.println(v); } while (0)
  #define DEBUG_FLUSH()           Serial.flush()
#else
  #define DEBUG_PRINT(x)          ((void)0)
  #define DEBUG_PRINTLN(x)        ((void)0)
  #define DEBUG_PRINT_VAL(lbl, v) ((void)0)
  #define DEBUG_FLUSH()           ((void)0)
#endif
