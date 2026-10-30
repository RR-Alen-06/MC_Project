// --- Pin Definitions ---
// PB12 is 5V-tolerant on STM32F103, safe for direct DO connection
#define GAS_SENSOR_PIN  PB12  
#define TRANSISTOR_PIN  PB14  // Base of NPN transistor via 1k resistor

// --- Logic Definition ---
// If the buzzer sounds in clean air, change LOW to HIGH below:
#define GAS_DETECTED    LOW   

void setup() {
  Serial.begin(115200);

  // Initialize pins
  pinMode(GAS_SENSOR_PIN, INPUT_PULLUP);
  pinMode(TRANSISTOR_PIN, OUTPUT);

  // Default state: Buzzer OFF
  digitalWrite(TRANSISTOR_PIN, LOW);

  Serial.println("=================================");
  Serial.println("  STM32 MQ-2 Gas Alarm Started   ");
  Serial.println("=================================");

  // Stabilization pause (heater warm-up)
  Serial.println("Warming up sensor coil (10s)...");
  delay(10000); 
  Serial.println("System ready. Monitoring...");
}

void loop() {
  int gasState = digitalRead(GAS_SENSOR_PIN);

  // Live debug print: prints 0 or 1
  Serial.print("Sensor Read: ");
  Serial.print(gasState);

  if (gasState == GAS_DETECTED) {
    digitalWrite(TRANSISTOR_PIN, HIGH); // Buzzer ON
    Serial.println("  --> [ALERT] Gas Detected! Buzzer ON");
  } else {
    digitalWrite(TRANSISTOR_PIN, LOW);  // Buzzer OFF
    Serial.println("  --> [NORMAL] Clean Air. Buzzer OFF");
  }

  delay(300);
}
