# Reverse Car Parking Assistant

An STM32 Blue Pill project that measures the distance to an obstacle with an HC-SR04 ultrasonic sensor. Three LEDs show the current distance zone, while a buzzer provides an audible warning as the vehicle approaches an obstacle.

## Hardware

- STM32F103C8T6 Blue Pill (or a compatible STM32 board using the same pin mapping)
- HC-SR04 ultrasonic distance sensor
- Green, yellow, and red LEDs, each with a current-limiting resistor
- Buzzer and an NPN transistor with a 1 kΩ base resistor
- Breadboard and jumper wires

Connect all components to a common ground. The buzzer is driven through the NPN transistor, not directly from an MCU pin. Check the voltage tolerance of your particular STM32 board's ECHO pin; use a level shifter or voltage divider if it is not 5 V tolerant.

## Wiring

| Component | STM32 pin | Notes |
| --- | --- | --- |
| HC-SR04 TRIG | PB10 | Sensor trigger output |
| HC-SR04 ECHO | PB11 | Sensor echo input; verify 5 V tolerance |
| Green LED | PA0 | Safe zone |
| Yellow LED | PA1 | Caution zone |
| Red LED | PA2 | Danger zone |
| Buzzer driver | PB14 | NPN transistor base through 1 kΩ |
| Built-in LED | PC13 | Active-low Blue Pill indicator |

Put a suitable current-limiting resistor in series with each external LED. Power the HC-SR04 according to its module specifications.

## Distance and alerts

| Measured distance | Indicator | Buzzer |
| --- | --- | --- |
| Greater than 30 cm | Green LED | Off |
| 15–30 cm | Yellow LED | Intermittent beep |
| Less than 15 cm | Red LED | Rapid beep |
| No valid echo | External LEDs off; built-in LED heartbeat | Off |

Measurements below approximately 2 cm are rejected as invalid by the firmware.

## Build and upload

1. Install the Arduino IDE and an STM32 board package that supports the STM32duino Arduino core.
2. Create an Arduino sketch for the STM32F103C8T6 and select the matching board and upload method.
3. Add [`Reverse Car Parking.cpp`](Reverse%20Car%20Parking.cpp) to the sketch folder alongside its `.ino` file.
4. Connect the board, compile, and upload the sketch.
5. With the sensor facing the area behind the vehicle, test the three distance zones and verify that the buzzer driver and LEDs behave as expected.

Board package names and upload options vary by Arduino IDE and board revision. Confirm the selected board's pin mapping before powering the circuit.
