# Smart Security Alarm System (Dual-Sensor Intruder Detection)

A perimeter intrusion and tampering detection system developed for the **STM32F103C8T6 (Blue Pill)** using the Arduino core for STM32 (STM32duino). The firmware monitors two independent sensor inputs: a **PIR motion sensor** and a **vibration/impact sensor**, activating a pulsed acoustic siren and visual alert when triggered.

---

## Hardware Requirements

- **Microcontroller**: STM32F103C8T6 "Blue Pill" (ARM Cortex-M3 @ 72 MHz)
- **Motion Sensor**: HC-SR501 PIR Motion Sensor Module
- **Vibration Sensor**: SW-420 Digital Vibration / Shock Sensor Module
- **Visual Indicators**:
  - 1x Green LED (Standby / Armed indicator)
  - 1x Red LED (Alarm indicator)
  - 2x 220 Ω current-limiting resistors
- **Acoustic Actuator**:
  - 5V Active Buzzer
  - 1x 2N2222 (or generic NPN) transistor driver with 1 kΩ base resistor
- **Prototyping**: Solderless breadboard and jumper wires

---

## Pinout and Hardware Wiring

| Peripheral / Signal | STM32 Pin | Function | Electrical Connection |
| :--- | :--- | :--- | :--- |
| **PIR Motion Sensor** | `PA0` | Digital Input | Digital output pin of the HC-SR501 PIR sensor (Active HIGH on motion). |
| **Vibration Sensor** | `PA1` | Digital Input | Digital output (DO) pin of the SW-420 sensor (Active HIGH on vibration). |
| **Green LED (Standby)** | `PA2` | Digital Output | Anode via 220 Ω series resistor to PA2; Cathode to GND. |
| **Red LED (Alarm)** | `PA3` | Digital Output | Anode via 220 Ω series resistor to PA3; Cathode to GND. |
| **Buzzer Driver** | `PA4` | Digital Output | Connected to NPN Transistor Base via 1 kΩ resistor to switch the buzzer. |
| **Sensor Power (VCC)** | `5V` / `3.3V` | Power Rail | HC-SR501 powered from 5V; SW-420 powered from 3.3V or 5V. |
| **Common Ground** | `GND` | Ground | All peripherals tied to common system ground. |

---

## Operational Logic

1. **Initialization & PIR Warm-Up**:
   - On boot, the system initializes GPIO modes for sensors and indicators.
   - Initial standby state is set: Green LED `HIGH` (ON), Red LED `LOW` (OFF), Buzzer `LOW` (OFF).
   - A 15-second stabilization delay (`delay(15000)`) is observed to allow the PIR pyroelectric sensor to reach thermal equilibrium and prevent false initial triggers.

2. **Perimeter Monitoring**:
   - The loop continuously reads both `PIR_PIN` (`PA0`) and `VIB_PIN` (`PA1`).
   - If either sensor detects a trigger condition (`pirVal == HIGH || vibVal == HIGH`), `triggerAlarm()` is immediately invoked.
   - Otherwise, the system remains in the armed standby state (Green LED ON, Red LED and Buzzer OFF).

3. **Alarm Sequence (`triggerAlarm`)**:
   - Green LED turns `LOW` (OFF).
   - Red LED turns `HIGH` (ON).
   - For a duration of 5,000 milliseconds (`ALARM_DURATION_MS`), the buzzer pulses with a 150 ms cadence (150 ms ON, 150 ms OFF).
   - Once the alarm duration elapses, the Red LED turns `LOW` (OFF) and the Green LED turns `HIGH` (ON), returning the system to armed standby.

---

## Build and Upload

1. Open [`Smart_Security_Alarm.ino`](Smart_Security_Alarm.ino) in the Arduino IDE (v2.x).
2. Verify that **STM32 MCU based boards** core is installed via the Boards Manager.
3. Select board settings:
   - **Board**: `Generic STM32F1 series`
   - **Board part number**: `BluePill F103C8` (or `STM32F103C8Tx`)
   - **Upload method**: `STLink` (recommended) or `STM32CubeProgrammer (Serial)`
4. Connect ST-Link V2 to the SWD pins (`SWCLK`, `SWDIO`, `GND`, `3.3V`) on the Blue Pill.
5. Click **Verify** to compile and **Upload** to flash the microcontroller.
