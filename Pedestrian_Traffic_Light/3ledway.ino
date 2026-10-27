//add#include <Arduino.h>

// ==========================================
// PIN DEFINITIONS
// ==========================================

// Vehicle 1 Traffic Light (North/South)
const int PIN_VEH1_RED    = PA0;
const int PIN_VEH1_YELLOW = PA1;
const int PIN_VEH1_GREEN  = PA2;

// Vehicle 2 Traffic Light (East/West)
const int PIN_VEH2_RED    = PA3;
const int PIN_VEH2_YELLOW = PA4;
const int PIN_VEH2_GREEN  = PA5;

// Pedestrian Crosswalk Signals
const int PIN_PED1_RED    = PA6;
const int PIN_PED1_GREEN  = PA7;
const int PIN_PED2_RED    = PB0;
const int PIN_PED2_GREEN  = PB1;

// Pedestrian Pushbuttons (Active HIGH with external 10k pull-downs)
const int PIN_BTN_PED1    = PB12;
const int PIN_BTN_PED2    = PB13;

// Onboard Heartbeat LED (Active LOW on STM32 Blue Pill)
const int PIN_HEARTBEAT   = PC13;

// ==========================================
// TIMING CONSTANTS (Milliseconds)
// ==========================================
const unsigned long BUTTON_WINDOW_TIME     = 30000; // 30-second window
const unsigned long MIN_VEHICLE_GREEN_TIME = 7000;  // Minimum green before allowing change
const unsigned long VEHICLE_YELLOW_TIME    = 3000;  // Yellow stopping duration
const unsigned long ALL_RED_CLEARANCE_TIME = 1500;  // All-red safety buffer
const unsigned long PEDESTRIAN_WALK_TIME   = 6000;  // Pedestrian walk green duration
const unsigned long PED_FLASH_DURATION     = 3000;  // Warning flash before walk ends
const unsigned long FLASH_INTERVAL         = 300;   // Green flash toggle interval
const unsigned long VEH_RELEASE_YELLOW     = 2000;  // Standalone Yellow duration before Green

// ==========================================
// FINITE STATE MACHINE (FSM) STATES
// ==========================================
enum TrafficState {
  STATE_VEHICLE_GREEN,    // Vehicles: GREEN | Pedestrians: RED
  STATE_VEHICLE_YELLOW,   // Vehicles: YELLOW only | Pedestrians: RED
  STATE_ALL_RED_PRE_PED,  // Vehicles: RED only | Pedestrians: RED (Safety gap)
  STATE_PEDESTRIAN_WALK,  // Vehicles: RED only | Pedestrians: GREEN
  STATE_PEDESTRIAN_FLASH, // Vehicles: RED only | Pedestrians: GREEN FLASHING
  STATE_ALL_RED_POST_PED, // Step 1 Release: RED only | Pedestrians: RED
  STATE_VEHICLE_RELEASE_Y // Step 2 Release: YELLOW only (Red turned OFF) | Pedestrians: RED
                          // Step 3 Release: Transitions to STATE_VEHICLE_GREEN
};

TrafficState currentState = STATE_VEHICLE_GREEN;

// ==========================================
// TRACKING VARIABLES
// ==========================================
unsigned long stateStartTime       = 0;
unsigned long windowStartTime      = 0;
unsigned long lastFlashToggle      = 0;
unsigned long lastHeartbeat        = 0;

int  pressCountInWindow            = 0;
bool pedestrianCallPending         = false;
bool lastBtn1Reading               = LOW;
bool lastBtn2Reading               = LOW;
bool flashState                    = false;
bool heartbeatState                = false;

// ==========================================
// OUTPUT HELPER
// ==========================================
void setOutputs(bool v1R, bool v1Y, bool v1G,
                bool v2R, bool v2Y, bool v2G,
                bool p1R, bool p1G,
                bool p2R, bool p2G) {
  digitalWrite(PIN_VEH1_RED,    v1R ? HIGH : LOW);
  digitalWrite(PIN_VEH1_YELLOW, v1Y ? HIGH : LOW);
  digitalWrite(PIN_VEH1_GREEN,  v1G ? HIGH : LOW);

  digitalWrite(PIN_VEH2_RED,    v2R ? HIGH : LOW);
  digitalWrite(PIN_VEH2_YELLOW, v2Y ? HIGH : LOW);
  digitalWrite(PIN_VEH2_GREEN,  v2G ? HIGH : LOW);

  digitalWrite(PIN_PED1_RED,    p1R ? HIGH : LOW);
  digitalWrite(PIN_PED1_GREEN,  p1G ? HIGH : LOW);
  digitalWrite(PIN_PED2_RED,    p2R ? HIGH : LOW);
  digitalWrite(PIN_PED2_GREEN,  p2G ? HIGH : LOW);
}

void transitionTo(TrafficState newState) {
  currentState = newState;
  stateStartTime = millis();
}

// ==========================================
// SETUP
// ==========================================
void setup() {
  // Vehicle 1 Pins
  pinMode(PIN_VEH1_RED,    OUTPUT);
  pinMode(PIN_VEH1_YELLOW, OUTPUT);
  pinMode(PIN_VEH1_GREEN,  OUTPUT);

  // Vehicle 2 Pins
  pinMode(PIN_VEH2_RED,    OUTPUT);
  pinMode(PIN_VEH2_YELLOW, OUTPUT);
  pinMode(PIN_VEH2_GREEN,  OUTPUT);

  // Pedestrian Pins
  pinMode(PIN_PED1_RED,    OUTPUT);
  pinMode(PIN_PED1_GREEN,  OUTPUT);
  pinMode(PIN_PED2_RED,    OUTPUT);
  pinMode(PIN_PED2_GREEN,  OUTPUT);

  // Heartbeat Indicator
  pinMode(PIN_HEARTBEAT,   OUTPUT);

  // Button Inputs (Active HIGH)
  pinMode(PIN_BTN_PED1,    INPUT);
  pinMode(PIN_BTN_PED2,    INPUT);

  // Initial State: Vehicles GREEN, Pedestrians RED
  windowStartTime = millis();
  transitionTo(STATE_VEHICLE_GREEN);
  setOutputs(false, false, true,   // Veh 1: Green
             false, false, true,   // Veh 2: Green
             true,  false,         // Ped 1: Red
             true,  false);        // Ped 2: Red
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
  unsigned long now = millis();

  // 1. Heartbeat LED monitor (500ms toggle)
  if (now - lastHeartbeat >= 500) {
    lastHeartbeat = now;
    heartbeatState = !heartbeatState;
    digitalWrite(PIN_HEARTBEAT, heartbeatState ? LOW : HIGH); // Active LOW on PC13
  }

  // 2. 30-Second Rolling Window (Max 2 button presses allowed)
  if (now - windowStartTime >= BUTTON_WINDOW_TIME) {
    windowStartTime = now;
    pressCountInWindow = 0;
  }

  // 3. Debounced Button Polling
  bool curBtn1 = digitalRead(PIN_BTN_PED1);
  bool curBtn2 = digitalRead(PIN_BTN_PED2);

  if ((curBtn1 == HIGH && lastBtn1Reading == LOW) || 
      (curBtn2 == HIGH && lastBtn2Reading == LOW)) {
    if (pressCountInWindow < 2) {
      pressCountInWindow++;
      pedestrianCallPending = true;
    }
    delay(50); // Contact debounce
  }
  lastBtn1Reading = curBtn1;
  lastBtn2Reading = curBtn2;

  // 4. Finite State Machine Transitions
  switch (currentState) {

    // VEHICLES: GREEN | PEDESTRIANS: RED
    case STATE_VEHICLE_GREEN:
      setOutputs(false, false, true,
                 false, false, true,
                 true,  false,
                 true,  false);

      if (pedestrianCallPending && (now - stateStartTime >= MIN_VEHICLE_GREEN_TIME)) {
        pedestrianCallPending = false;
        transitionTo(STATE_VEHICLE_YELLOW);
      }
      break;

    // VEHICLES: YELLOW ONLY | PEDESTRIANS: RED
    case STATE_VEHICLE_YELLOW:
      setOutputs(false, true,  false,
                 false, true,  false,
                 true,  false,
                 true,  false);

      if (now - stateStartTime >= VEHICLE_YELLOW_TIME) {
        transitionTo(STATE_ALL_RED_PRE_PED);
      }
      break;

    // ALL SIGNALS: RED
    case STATE_ALL_RED_PRE_PED:
      setOutputs(true, false, false,
                 true, false, false,
                 true, false,
                 true, false);

      if (now - stateStartTime >= ALL_RED_CLEARANCE_TIME) {
        transitionTo(STATE_PEDESTRIAN_WALK);
      }
      break;

    // VEHICLES: RED ONLY | PEDESTRIANS: WALK (GREEN)
    case STATE_PEDESTRIAN_WALK:
      setOutputs(true, false, false,
                 true, false, false,
                 false, true,
                 false, true);

      if (now - stateStartTime >= PEDESTRIAN_WALK_TIME) {
        lastFlashToggle = now;
        flashState = true;
        transitionTo(STATE_PEDESTRIAN_FLASH);
      }
      break;

    // VEHICLES: RED ONLY | PEDESTRIANS: GREEN FLASHING
    case STATE_PEDESTRIAN_FLASH:
      if (now - lastFlashToggle >= FLASH_INTERVAL) {
        lastFlashToggle = now;
        flashState = !flashState;
      }

      setOutputs(true, false, false,
                 true, false, false,
                 false, flashState,
                 false, flashState);

      if (now - stateStartTime >= PED_FLASH_DURATION) {
        transitionTo(STATE_ALL_RED_POST_PED);
      }
      break;

    // RELEASE STEP 1: VEHICLES HELD AT RED ONLY (Pedestrians return to RED)
    case STATE_ALL_RED_POST_PED:
      setOutputs(true, false, false,   // Veh 1: Red ONLY
                 true, false, false,   // Veh 2: Red ONLY
                 true, false,          // Ped 1: Red
                 true, false);         // Ped 2: Red

      if (now - stateStartTime >= ALL_RED_CLEARANCE_TIME) {
        transitionTo(STATE_VEHICLE_RELEASE_Y);
      }
      break;

    // RELEASE STEP 2: VEHICLES SWITCH TO YELLOW ONLY (Red turns OFF)
    case STATE_VEHICLE_RELEASE_Y:
      setOutputs(false, true, false,   // Veh 1: Yellow ONLY
                 false, true, false,   // Veh 2: Yellow ONLY
                 true,  false,         // Ped 1: Red
                 true,  false);        // Ped 2: Red

      if (now - stateStartTime >= VEH_RELEASE_YELLOW) {
        // RELEASE STEP 3: TRANSITION DIRECTLY TO GREEN
        transitionTo(STATE_VEHICLE_GREEN);
      }
      break;
  }
}