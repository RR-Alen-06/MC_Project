const int potPin = PA0;  // Potentiometer wiper
const int ledPin = PA8;  // Timer 1 Channel 1 (PWM)

void setup() {
  // Set 12-bit resolution (0 to 4095)
  analogReadResolution(12);
  analogWriteResolution(12);
 
  // NOTE: Do NOT call pinMode(ledPin, OUTPUT); analogWrite handles PWM alternate functions.
}

void loop() {
  int potRaw = analogRead(potPin); // Range: 0 to 4095

  // Deadband: If pot is near minimum, force it completely OFF
  int pwmValue = 0;
  if (potRaw > 40) {
    pwmValue = potRaw;
  }

  analogWrite(ledPin, constrain(pwmValue, 0, 4095));
  delay(15);
}
