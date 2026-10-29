#define PIR_PIN       PA0
#define VIB_PIN       PA1
#define LED_GREEN     PA2
#define LED_RED       PA3
#define BUZZER_PIN    PA4

const uint32_t ALARM_DURATION_MS = 5000;

void setup() {
  pinMode(PIR_PIN, INPUT);
  pinMode(VIB_PIN, INPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Standby initial state
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // PIR sensor warm-up stabilization window
  delay(15000);
}

void loop() {
  int pirVal = digitalRead(PIR_PIN);
  int vibVal = digitalRead(VIB_PIN);

  // Trigger if PIR detects motion or vibration sensor detects impact
  if (pirVal == HIGH || vibVal == HIGH) {
    triggerAlarm();
  } else {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void triggerAlarm() {
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, HIGH);

  uint32_t startTime = millis();
  while (millis() - startTime < ALARM_DURATION_MS) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(150);
    digitalWrite(BUZZER_PIN, LOW);
    delay(150);
  }

  // Return to Armed/Standby
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, HIGH);
}
