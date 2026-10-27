#include <Arduino.h>

// --- PIN CONFIGURATION ---
// HC-SR04 Pins (Matches your working test circuit)
#define TRIG_PIN   PB10
#define ECHO_PIN   PB11 // 5V-tolerant pin

// Status Indicator LEDs
#define GREEN_LED  PA0  // Safe (> 30 cm)
#define YELLOW_LED PA1  // Caution (15 - 30 cm)
#define RED_LED    PA2  // Danger (< 15 cm)

// Alert Outputs
#define BUZZER_PIN PB14 // NPN transistor base via 1k resistor
#define BOARD_LED  PC13 // Built-in Blue Pill LED (Active LOW)

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BOARD_LED, OUTPUT);

  // Initialize all outputs to OFF state
  digitalWrite(TRIG_PIN, LOW);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(BOARD_LED, HIGH); // HIGH turns OFF PC13
}

// Reliable direct pulse measurement (No pulseIn)
float measureDistanceCM() {
  // 1. Send clean 10us ultrasonic burst
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(4);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // 2. Wait for ECHO pin to go HIGH (Timeout counter ~30ms)
  uint32_t timeout = 50000;
  while (digitalRead(ECHO_PIN) == LOW && timeout > 0) {
    timeout--;
  }

  // Echo never started (Sensor disconnected, miswired, or failed)
  if (timeout == 0) {
    return -1.0;
  }

  // 3. Measure duration while ECHO pin stays HIGH
  uint32_t pulse_start = micros();
  while (digitalRead(ECHO_PIN) == HIGH && (micros() - pulse_start) < 30000);
  uint32_t duration = micros() - pulse_start;

  // 4. Glitch filter: duration < 120us corresponds to < 2 cm deadband
  if (duration < 120 || duration >= 30000) {
    return -1.0;
  }

  // Distance calculation: speed of sound = 0.0343 cm/us
  return (duration * 0.0343) / 2.0;
}

void loop() {
  float distance = measureDistanceCM();

  // --- FAULT / TIMEOUT / OUT-OF-RANGE ---
  if (distance < 0) {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER_PIN, LOW);

    // Heartbeat blink on board LED to indicate active search
    digitalWrite(BOARD_LED, LOW);
    delay(80);
    digitalWrite(BOARD_LED, HIGH);
    delay(80);
    return;
  }

  // --- ZONE 1: SAFE (> 30 cm) ---
  if (distance > 30.0) {
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(BOARD_LED, HIGH); // Off
    delay(60);
  }

  // --- ZONE 2: CAUTION (15 cm to 30 cm) ---
  else if (distance >= 15.0 && distance <= 30.0) {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, HIGH);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BOARD_LED, HIGH);

    // Intermittent warning beep
    digitalWrite(BUZZER_PIN, HIGH);
    delay(70);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }

  // --- ZONE 3: DANGER / CRITICAL (< 15 cm) ---
  else {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BOARD_LED, LOW); // Onboard LED active warning

    // Rapid alert beep
    digitalWrite(BUZZER_PIN, HIGH);
    delay(40);
    digitalWrite(BUZZER_PIN, LOW);
    delay(40);
  }
}