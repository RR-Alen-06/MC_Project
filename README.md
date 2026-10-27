# Microcontroller (STM32 / Arduino) Projects Hub

A structured collection of embedded systems and microcontroller projects developed for the **STM32F103C8T6 (Blue Pill)** using the Arduino core for STM32 (STM32duino). Each project contains production-ready firmware, pinout tables, hardware wiring guides, and operational details.

---

## Projects Directory

| Project | Target MCU | Core Sensors & Actuators | Folder | Documentation |
| :--- | :--- | :--- | :--- | :--- |
| **Reverse Car Parking Assistant** | STM32F103C8T6 (Blue Pill) | HC-SR04 Ultrasonic Sensor, 3x Distance LEDs, NPN-driven Buzzer | [`Reverse_Car_Parking/`](Reverse_Car_Parking/) | [Wiring & Logic Guide](Reverse_Car_Parking/README.md) |
| **Pedestrian Crosswalk Signal Controller** | STM32F103C8T6 (Blue Pill) | Dual vehicle traffic lights, pedestrian signals, crosswalk pushbuttons, FSM | [`Pedestrian_Traffic_Light/`](Pedestrian_Traffic_Light/) | [Phases & Pinout Guide](Pedestrian_Traffic_Light/README.md) |
| **Adaptive LED Dimmer & Night-Light** | STM32F103C8T6 (Blue Pill) | LDR Sensor, 10k Potentiometer, 12-bit PWM LED, Pushbutton, Gamma Curve | [`Adaptive_LED_Dimmer/`](Adaptive_LED_Dimmer/) | [Modes & Pinout Guide](Adaptive_LED_Dimmer/README.md) |

---

## Repository Structure

```text
MC_Project/
├── README.md                          # Overarching project index and hardware overview
├── Reverse_Car_Parking/               # Ultrasonic parking distance assistant
│   ├── README.md                      # Wiring diagram, distance thresholds, and build steps
│   └── Reverse Car Parking.cpp        # STM32 firmware for distance sensing and alert zones
├── Pedestrian_Traffic_Light/          # Crosswalk and traffic light finite-state machine
│   ├── README.md                      # Signal timing, pushbutton debounce, and safety notes
│   └── 3ledway.ino                    # Arduino sketch with full state machine implementation
└── Adaptive_LED_Dimmer/               # Dual-mode PWM dimmer with auto-calibrating LDR
    ├── README.md                      # Gamma curve math, pin mapping, and build steps
    └── Adaptive_LED_Dimmer.ino        # 12-bit ADC/PWM sketch with mode debounce
```

---

## Hardware & Architecture Overview

All projects are engineered around the **STM32F103C8T6 ARM Cortex-M3** microcontroller:
- **Core Clock**: 72 MHz
- **Logic Level**: 3.3 V (Note: Ensure 5 V tolerance is verified on sensor inputs such as the HC-SR04 ECHO pin using level shifters or voltage dividers where required)
- **High-Resolution ADC & PWM**: 12-bit resolution (0–4095) for smooth analog control and duty cycles.
- **Actuator Driving**: High-current loads (such as buzzers) are switched via an external NPN transistor driver rather than drawn directly from GPIO pins.
- **Visual Status**: All projects maintain an active-low heartbeat or mode status on the onboard indicator LED (`PC13`).

---

## Development Environment & Toolchain

To compile and upload any project sketch to the STM32 Blue Pill:

1. **IDE**: Install the [Arduino IDE](https://www.arduino.cc/en/software) (version 2.x recommended).
2. **STM32 Board Support**:
   - In Arduino IDE Preferences, add the STM32 board manager URL:
     ```text
     https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json
     ```
   - Open **Boards Manager** and install **STM32 MCU based boards**.
3. **Board Selection**:
   - Board: `Generic STM32F1 series`
   - Board part number: `BluePill F103C8` (or `STM32F103C8Tx`)
   - Upload method: `STLink` (recommended) or `STM32CubeProgrammer (Serial)`
4. Navigate to the respective project directory and follow the project-specific README instructions.

---

## Safety & Prototyping

These implementations are educational and experimental prototypes. Circuits should be verified on a solderless breadboard with appropriate current-limiting resistors before connecting to power sources or live physical installations.
