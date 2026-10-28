# 12-Bit Potentiometer PWM LED Dimmer

A minimalist, high-resolution manual LED dimmer built on the **STM32F103C8T6 (Blue Pill)** using STM32duino. It takes advantage of the STM32's native 12-bit Analog-to-Digital Converter (ADC) and 12-bit hardware timer PWM output, offering 4,096 discrete brightness levels (compared to standard 8-bit / 256-level Arduino dimmers).

---

## Hardware Requirements

- **Microcontroller**: STM32F103C8T6 "Blue Pill" (ARM Cortex-M3 @ 72 MHz)
- **Manual Control**: 10 kΩ linear potentiometer
- **LED Output**: High-brightness LED with a 220 Ω current-limiting resistor
- **Prototyping**: Solderless breadboard and jumper wires

---

## Pinout and Wiring

| Component | STM32 Pin | Function | Electrical Connection |
| :--- | :--- | :--- | :--- |
| **Potentiometer Wiper** | `PA0` | ADC1_IN0 | Center wiper pin of the 10 kΩ pot (analog input: 0 to 4095). |
| **Potentiometer Leg 1** | `3.3V` | Power | Positive reference voltage rail. |
| **Potentiometer Leg 2** | `GND` | Ground | System common ground. |
| **LED Anode (+)** | `PA8` | TIM1_CH1 | 12-bit hardware timer PWM output via a 220 Ω series resistor. |
| **LED Cathode (-)** | `GND` | Ground | Connected to common ground. |

> [!NOTE]
> Do not explicitly configure `pinMode(ledPin, OUTPUT)`. On the STM32duino core, `analogWrite()` automatically handles timer peripheral routing and GPIO alternate-function assignment for `PA8`.

---

## Key Features & Logic

- **12-Bit Resolution**: Both ADC sampling and PWM generation are configured for 12-bit resolution via `analogReadResolution(12)` and `analogWriteResolution(12)`.
- **Zero-Point Deadband Cutoff**: Potentiometer hardware tolerances and ADC noise can often produce small non-zero readings near the mechanical stop. A software threshold (`potRaw > 40`) guarantees that turning the dial fully counter-clockwise completely shuts off the LED without residual glow or flicker.
- **Fast Loop Rate**: A 15 ms update delay delivers a ~66 Hz refresh loop, giving instantaneous tactile feedback as the dial is turned.

---

## Build and Upload

1. Open [`Potentiometer_PWM_Dimmer.ino`](Potentiometer_PWM_Dimmer.ino) in the Arduino IDE (v2.x).
2. Verify that **STM32 MCU based boards** core is installed.
3. Select board: `Generic STM32F1 series` -> `BluePill F103C8`.
4. Select your upload method (e.g., `STLink` or `STM32CubeProgrammer (Serial)`).
5. Compile and flash the Blue Pill.
