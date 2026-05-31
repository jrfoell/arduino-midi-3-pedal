// calibration.h
// Calibration data structure, Flash persistence, NeoPixel helpers, and the
// startup-triggered calibration sequence.

#pragma once

#include <Adafruit_NeoPixel.h>
#include <FlashStorage_SAMD.h>

// ─── Pin assignments ─────────────────────────────────────────────────────────
#define PIN_TIP          A2   // Right / damper pedal potentiometer wiper
#define PIN_RING         A3   // Middle + left pedal switch network

// ─── Default calibration (measured in initial bench test) ────────────────────
// Used on first boot before a calibration run has been saved to EEPROM.
#define DEFAULT_RING_BASELINE    60
#define DEFAULT_RING_MIDDLE    1950
#define DEFAULT_RING_LEFT      2450
#define DEFAULT_RING_BOTH      2850
#define DEFAULT_TIP_REST       3950
#define DEFAULT_TIP_PRESSED    1850

// ─── Calibration timing ──────────────────────────────────────────────────────
#define CALIB_HOLD_MS       2000   // Pedal hold duration at boot to trigger calibration (ms)
#define CALIB_READ_MS       2000   // Sampling window per pedal state (ms)
#define CALIB_SLOW_BLINK_MS  500   // Blink half-period: waiting for press
#define CALIB_FAST_BLINK_MS  100   // Blink half-period: signal user to release
#define CALIB_PRESS_MARGIN   300   // Min ADC delta from baseline to detect a press

// ─── Calibration data ────────────────────────────────────────────────────────

static const uint32_t CALIB_MAGIC       = 0xFEED1234;
static const int      CALIB_EEPROM_ADDR = 0;

struct CalibrationData {
  uint32_t magic;
  int ringBaseline;   // Ring ADC: all pedals released
  int ringMiddle;     // Ring ADC: middle / sostenuto only
  int ringLeft;       // Ring ADC: left / soft only
  int ringBoth;       // Ring ADC: middle + left together
  int tipRest;        // Tip ADC: damper fully released
  int tipPressed;     // Tip ADC: damper fully pressed
};

// Midpoint thresholds derived from stored measurements — used by decode logic.
inline int threshNoneToMiddle(const CalibrationData& d) {
  return (d.ringBaseline + d.ringMiddle) / 2;
}
inline int threshMiddleToLeft(const CalibrationData& d) {
  return (d.ringMiddle + d.ringLeft) / 2;
}
inline int threshLeftToBoth(const CalibrationData& d) {
  return (d.ringLeft + d.ringBoth) / 2;
}

// ─── EEPROM ──────────────────────────────────────────────────────────────────

inline CalibrationData defaultCalibration() {
  CalibrationData d;
  d.magic        = CALIB_MAGIC;
  d.ringBaseline = DEFAULT_RING_BASELINE;
  d.ringMiddle   = DEFAULT_RING_MIDDLE;
  d.ringLeft     = DEFAULT_RING_LEFT;
  d.ringBoth     = DEFAULT_RING_BOTH;
  d.tipRest      = DEFAULT_TIP_REST;
  d.tipPressed   = DEFAULT_TIP_PRESSED;
  return d;
}

// Returns true if stored data has a valid magic number.
inline bool loadCalibration(CalibrationData& data) {
  EEPROM.get(CALIB_EEPROM_ADDR, data);
  return data.magic == CALIB_MAGIC;
}

inline void saveCalibration(const CalibrationData& data) {
  EEPROM.put(CALIB_EEPROM_ADDR, data);
  EEPROM.commit();  // required by FlashStorage_SAMD to flush buffer to NVM
}

// ─── NeoPixel helper ─────────────────────────────────────────────────────────

static void neoSet(Adafruit_NeoPixel& px, uint32_t color) {
  px.setPixelColor(0, color);
  px.show();
}

// ─── Calibration sequence ────────────────────────────────────────────────────

// Runs one pedal phase of the calibration sequence:
//   - Slow-blinks COLOR while waiting for the pedal to be pressed
//   - Samples PIN for CALIB_READ_MS once the press is detected
//   - Fast-blinks to signal the user to release
//   - Returns the averaged ADC reading
//
// pressedAbove=true  → "pressed" means value exceeds pressThreshold (Ring pedals)
// pressedAbove=false → "pressed" means value falls below pressThreshold (Tip)
static int calibPedalPhase(Adafruit_NeoPixel& px, uint32_t color,
                            int pin, int pressThreshold, bool pressedAbove) {
  long sampleSum   = 0;
  int  sampleCount = 0;
  unsigned long sampleStart = 0;
  bool pressed      = false;
  bool samplingDone = false;

  unsigned long blinkTimer = millis();
  bool blinkOn = true;
  neoSet(px, color);

  while (true) {
    unsigned long now = millis();
    int  val       = analogRead(pin);
    bool isPressed = pressedAbove ? (val > pressThreshold) : (val < pressThreshold);

    // Slow blink while waiting/holding; fast blink once sampling is complete
    unsigned long interval = samplingDone ? CALIB_FAST_BLINK_MS : CALIB_SLOW_BLINK_MS;
    if (now - blinkTimer >= interval) {
      blinkOn = !blinkOn;
      neoSet(px, blinkOn ? color : 0);
      blinkTimer = now;
    }

    if (!pressed && isPressed) {
      pressed     = true;
      sampleStart = now;
    }

    if (pressed && !samplingDone) {
      sampleSum += val;
      sampleCount++;
      if (now - sampleStart >= CALIB_READ_MS) {
        samplingDone = true;
        blinkTimer   = now;  // reset so fast-blink starts cleanly
      }
    }

    if (samplingDone && !isPressed) break;  // pedal released → advance
  }

  neoSet(px, 0);
  delay(400);
  return (sampleCount > 0) ? (int)(sampleSum / sampleCount) : pressThreshold;
}

// Returns true if a Ring pedal is held continuously for CALIB_HOLD_MS at boot.
// NeoPixel feedback: solid blue during the hold window, then fast-blink blue
// prompting the user to release. Returns false if the pedal is released before
// CALIB_HOLD_MS elapses.
// Call once in setup() after calibration data has been loaded or defaulted.
inline bool pedalHeldAtBoot(const CalibrationData& calData, Adafruit_NeoPixel& px) {
  int threshold = calData.ringBaseline + CALIB_PRESS_MARGIN;
  const uint32_t BLUE = px.Color(0, 0, 200);

  if (analogRead(PIN_RING) <= threshold) return false;

  // Solid blue while hold window counts down
  neoSet(px, BLUE);
  unsigned long start = millis();
  while (millis() - start < CALIB_HOLD_MS) {
    if (analogRead(PIN_RING) <= threshold) {
      neoSet(px, 0);
      return false;
    }
    delay(20);
  }

  // Fast-blink blue — prompt user to release to begin calibration
  unsigned long blinkTimer = millis();
  bool blinkOn = true;
  while (analogRead(PIN_RING) > threshold) {
    unsigned long now = millis();
    if (now - blinkTimer >= CALIB_FAST_BLINK_MS) {
      blinkOn = !blinkOn;
      neoSet(px, blinkOn ? BLUE : 0);
      blinkTimer = now;
    }
    delay(5);
  }

  neoSet(px, 0);
  return true;
}

// Full calibration sequence. Right-to-left pedal order.
// Saves completed data to EEPROM and signals done with green blinks.
static void runCalibration(Adafruit_NeoPixel& px, CalibrationData& data) {
  const uint32_t BLUE   = px.Color(0,   0,   200);
  const uint32_t VIOLET = px.Color(148, 0,   211);
  const uint32_t RED    = px.Color(200, 0,   0  );
  const uint32_t ORANGE = px.Color(220, 80,  0  );
  const uint32_t YELLOW = px.Color(220, 180, 0  );
  const uint32_t GREEN  = px.Color(0,   200, 0  );

  // Wait for any held pedal to be released before sampling the baseline.
  // Necessary because calibration is triggered by holding a pedal at boot.
  while (analogRead(PIN_RING) > (data.ringBaseline + CALIB_PRESS_MARGIN)) {
    delay(10);
  }
  delay(100);

  // Phase 1: Solid blue — sample Ring and Tip with all pedals at rest
  neoSet(px, BLUE);
  {
    long ringSum = 0, tipSum = 0;
    int  count = 0;
    unsigned long start = millis();
    while (millis() - start < CALIB_READ_MS) {
      ringSum += analogRead(PIN_RING);
      tipSum  += analogRead(PIN_TIP);
      count++;
      delay(5);
    }
    data.ringBaseline = (int)(ringSum / count);
    data.tipRest      = (int)(tipSum  / count);
  }
  neoSet(px, 0);
  delay(400);

  // Phase 2: Violet — right / damper pedal (Tip falls when pressed)
  data.tipPressed = calibPedalPhase(px, VIOLET,
                                     PIN_TIP,
                                     data.tipRest - CALIB_PRESS_MARGIN,
                                     false);

  // Phase 3: Red — middle / sostenuto pedal (Ring rises when pressed)
  data.ringMiddle = calibPedalPhase(px, RED,
                                     PIN_RING,
                                     data.ringBaseline + CALIB_PRESS_MARGIN,
                                     true);

  // Phase 4: Orange — left / soft pedal (Ring rises when pressed)
  data.ringLeft = calibPedalPhase(px, ORANGE,
                                   PIN_RING,
                                   data.ringBaseline + CALIB_PRESS_MARGIN,
                                   true);

  // Phase 5: Yellow — middle + left together
  // Threshold set above ringLeft so a single pedal doesn't accidentally trigger
  data.ringBoth = calibPedalPhase(px, YELLOW,
                                   PIN_RING,
                                   data.ringLeft + CALIB_PRESS_MARGIN / 2,
                                   true);

  // Phase 6: Blink green while writing to EEPROM, then off
  data.magic = CALIB_MAGIC;
  saveCalibration(data);
  for (int i = 0; i < 6; i++) {
    neoSet(px, (i % 2 == 0) ? GREEN : 0);
    delay(200);
  }
  neoSet(px, 0);
}

