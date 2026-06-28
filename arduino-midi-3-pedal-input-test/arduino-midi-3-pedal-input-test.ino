// Calibration sketch — prints raw ADC readings for all three pedals.
// Upload, open Serial Monitor at 115200 baud, and press each pedal
// (alone and in combination) to observe the ADC counts and voltages.

const int PIN_TIP  = A2;  // Right / Damper potentiometer wiper
const int PIN_RING = A3;  // Middle + Left switch network

const float V_REF      = 3.3;
const int   ADC_MAX    = 4095;  // 12-bit ADC on Feather M4
const int   PRINT_MS   = 1000;   // print interval in milliseconds

void setup() {
  Serial.begin(115200);
  while (!Serial) {}  // wait for USB serial to connect

  analogReadResolution(12);

  pinMode(PIN_TIP,  INPUT);
  pinMode(PIN_RING, INPUT);

  Serial.println("=== Pedal calibration ===");
  Serial.println("Press each pedal alone, then in combination.");
  Serial.println("Columns: TIP_raw  TIP_V  |  RING_raw  RING_V");
  Serial.println();
}

void loop() {
  static unsigned long lastPrint = 0;
  unsigned long now = millis();

  if (now - lastPrint >= PRINT_MS) {
    lastPrint = now;

    int   tipRaw  = analogRead(PIN_TIP);
    int   ringRaw = analogRead(PIN_RING);
    float tipV    = tipRaw  * V_REF / ADC_MAX;
    float ringV   = ringRaw * V_REF / ADC_MAX;

    Serial.print("TIP  ");
    Serial.print(tipRaw);
    Serial.print("\t");
    Serial.print(tipV, 2);
    Serial.print(" V");

    Serial.print("   |   RING  ");
    Serial.print(ringRaw);
    Serial.print("\t");
    Serial.print(ringV, 2);
    Serial.println(" V");
  }
}
