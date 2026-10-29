// ============================================================================
// Automatic Water Tank Controller
// STM32 Blue Pill (STM32F103C8T6) | Arduino / STM32duino Core
// ============================================================================
//
// WIRING SUMMARY:
// - Water Sensor : (+) -> 3.3V, (-) -> GND, (S) -> PA0
// - Status LEDs  : Red -> PA2, Yellow -> PA3, Green -> PA4 (each via 220R to GND)
// - Relay Module : VCC -> 3.3V, GND -> GND, IN -> PB10
// - DC Pump      : Powered via Relay COM and NO (Normally Open) terminals
//
// BAR-GRAPH LED & MOTOR HYSTERESIS LOGIC:
// - Low  (< 1365)      : Red ON               | Motor turns ON  (Starts filling)
// - Mid  (1365 - 1799) : Red + Yellow ON      | Motor stays ON if rising from Red;
//                                               Motor stays OFF if falling from Green
// - Full (>= 1800)     : Red + Yellow + Green | Motor turns OFF (Tank full)
// ============================================================================

const int SENSOR_PIN  = PA0;   // Analog Water Level Sensor (ADC1 Ch 0)
const int RELAY_PIN   = PB10;  // Relay IN control pin
const int RED_LED     = PA2;   // Low Water Level LED
const int YELLOW_LED  = PA3;   // Mid Water Level LED
const int GREEN_LED   = PA4;   // Full Water Level LED

const int LOW_THRESHOLD  = 1365; // Pump triggers ON below this ADC reading
const int FULL_THRESHOLD = 1800; // Pump triggers OFF at or above this ADC reading

const bool RELAY_ACTIVE_LOW = true;

bool pumpOn = false; // Starts OFF on boot (if powered up in Yellow, waits until Red)

void setPump(bool state) {
  pumpOn = state;
  // Active-LOW relay (with Relay VCC on 3.3V): LOW (0V) = ON, HIGH (3.3V) = OFF
  bool pinLevel = RELAY_ACTIVE_LOW ? (state ? LOW : HIGH) : (state ? HIGH : LOW);
  digitalWrite(RELAY_PIN, pinLevel);
}

void setLeds(bool red, bool yellow, bool green) {
  digitalWrite(RED_LED,    red    ? HIGH : LOW);
  digitalWrite(YELLOW_LED, yellow ? HIGH : LOW);
  digitalWrite(GREEN_LED,  green  ? HIGH : LOW);
}

int readLevel() {
  long total = 0;
  for (int i = 0; i < 8; i++) {
    total += analogRead(SENSOR_PIN);
    delay(2);
  }
  return (int)(total / 8);
}

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  setPump(false); // Ensure motor is OFF on boot

  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  pinMode(SENSOR_PIN, INPUT_ANALOG);
  analogReadResolution(12);

  setLeds(false, false, false);
}

void loop() {
  int waterLevel = readLevel();

  if (waterLevel < LOW_THRESHOLD) {
    // Tank Low: Turn motor ON, light Red LED only
    setPump(true);
    setLeds(true, false, false);
  } 
  else if (waterLevel >= FULL_THRESHOLD) {
    // Tank Full: Turn motor OFF, light all 3 LEDs (Red + Yellow + Green)
    setPump(false);
    setLeds(true, true, true);
  } 
  else {
    // Mid / Yellow Hysteresis Zone (1365 to 1799):
    // - If rising from Red, pumpOn is true  -> Motor stays ON
    // - If falling from Green, pumpOn is false -> Motor stays OFF
    setPump(pumpOn);
    setLeds(true, true, false); // Bar-graph: Red + Yellow ON, Green OFF
  }

  delay(100);
}
