// --- Pin Configuration ---
const int buttonPin = PB13; // Mode switch button (Active LOW)
const int potPin    = PA0;  // Potentiometer input (ADC1_IN0)
const int ldrPin    = PA1;  // LDR voltage divider input (ADC1_IN1)
const int ledPin    = PA8;  // PWM output (TIM1_CH1)
const int statusLed = PC13; // Onboard LED indicator

// --- Mode State ---
// 0 = Mode 1: Auto LDR (Night-light with Gamma Curve)
// 1 = Mode 2: Manual Potentiometer
int currentMode = 0;

// --- Button Debounce Variables ---
int lastButtonReading = HIGH;
int stableButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50; // 50 ms debounce filter

// --- LDR Dynamic Calibration Variables ---
int minLdr = 4095; // Tracks the darkest light level seen
int maxLdr = 0;    // Tracks the brightest light level seen

void setup() {
  // PB13 uses internal pull-up: unpressed = HIGH, pressed to GND = LOW
  pinMode(buttonPin, INPUT_PULLUP);
  
  // PC13 is active-LOW on the Blue Pill
  pinMode(statusLed, OUTPUT);
  digitalWrite(statusLed, HIGH); // Start OFF in Mode 0

  // 12-bit ADC and PWM resolution (0 to 4095)
  analogReadResolution(12);
  analogWriteResolution(12);

  // NOTE: Do not call pinMode(ledPin, OUTPUT) — analogWrite assigns timer PWM automatically.
}

void loop() {
  // -------------------------------------------------------------
  // 1. Debounce Push Button & Toggle Operating Mode
  // -------------------------------------------------------------
  int reading = digitalRead(buttonPin);

  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != stableButtonState) {
      stableButtonState = reading;

      // On button press (falling edge: HIGH -> LOW)
      if (stableButtonState == LOW) {
        currentMode = !currentMode; // Toggle mode between 0 and 1

        // Update onboard PC13 LED: OFF = Mode 0 (LDR), ON = Mode 1 (Pot)
        digitalWrite(statusLed, currentMode ? LOW : HIGH);
      }
    }
  }
  lastButtonReading = reading;

  // -------------------------------------------------------------
  // 2. Compute Target PWM Based on Active Mode
  // -------------------------------------------------------------
  int finalPwm = 0;

  if (currentMode == 0) {
    // === MODE 1: LDR AUTO BRIGHTNESS ===
    int ldrRaw = analogRead(ldrPin);

    // Dynamic auto-calibration tracking
    if (ldrRaw < minLdr) minLdr = ldrRaw;
    if (ldrRaw > maxLdr) maxLdr = ldrRaw;

    // Safety margins to ensure full off and full bright reachability
    int brightBound = maxLdr - 100;
    int darkBound   = minLdr + 100;

    // Map light reading to linear intensity (0 = Room Lit, 1000 = Covered/Dark)
    long linearBrightness = map(ldrRaw, brightBound, darkBound, 0, 1000);
    linearBrightness = constrain(linearBrightness, 0, 1000);

    // Quadratic gamma curve for eye perception: PWM = (intensity / 1000)^2 * 4095
    long gammaPwm = (linearBrightness * linearBrightness * 4095L) / (1000L * 1000L);

    if (linearBrightness <= 0) {
      finalPwm = 0;
    } else {
      finalPwm = (int)gammaPwm;
    }
  } 
  else {
    // === MODE 2: POTENTIOMETER MANUAL CONTROL ===
    int potRaw = analogRead(potPin);

    // Deadband threshold to guarantee 0% output when turned fully counter-clockwise
    if (potRaw > 40) {
      finalPwm = potRaw;
    } else {
      finalPwm = 0;
    }
  }

  // -------------------------------------------------------------
  // 3. Write Duty Cycle to LED
  // -------------------------------------------------------------
  analogWrite(ledPin, constrain(finalPwm, 0, 4095));

  delay(15); // Loop execution rate (~66 Hz)
}
