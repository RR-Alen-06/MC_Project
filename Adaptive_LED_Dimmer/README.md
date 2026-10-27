# Adaptive LED Dimmer & Night-Light Controller

An STM32 Blue Pill project providing dual-mode 12-bit PWM LED brightness control. It dynamically transitions between an automated adaptive ambient light sensor (LDR with eye-perceived quadratic gamma correction) and manual potentiometer dial control via a debounced pushbutton.

---

## Hardware Requirements

- **Microcontroller**: STM32F103C8T6 "Blue Pill" (ARM Cortex-M3 @ 72 MHz)
- **Light Sensor**: Light Dependent Resistor (LDR) in a voltage divider with a 10 kΩ resistor
- **Manual Input**: 10 kΩ linear potentiometer
- **Mode Toggle**: Momentary tactile pushbutton (uses internal pull-up)
- **Output Indicator**: High-brightness LED with a 220 Ω current-limiting resistor
- **Prototyping**: Solderless breadboard and jumper wires

---

## Pinout and Wiring

| Function | STM32 Pin | Channel / Peripheral | Electrical Notes |
| :--- | :--- | :--- | :--- |
| **Mode Switch Button** | `PB13` | Digital Input | Active LOW; uses MCU internal pull-up (`INPUT_PULLUP`). Connect other leg to GND. |
| **Potentiometer Wiper** | `PA0` | ADC1_IN0 | 12-bit analog input (0–4095). Outer legs connect to 3.3V and GND. |
| **LDR Sensor Input** | `PA1` | ADC1_IN1 | 12-bit analog input (0–4095). Voltage divider with 10 kΩ pull-down to GND. |
| **LED PWM Output** | `PA8` | TIM1_CH1 | 12-bit timer PWM (0–4095). Connect to LED anode via 220 Ω resistor; cathode to GND. |
| **Status Indicator** | `PC13` | Onboard LED | Active LOW indicator showing current mode. |

---

## Operating Modes

Operating modes toggle seamlessly on each press of the `PB13` push button:

### Mode 0: Auto LDR Night-Light (Default)
- **Onboard Status LED (`PC13`)**: **OFF**
- **Dynamic Auto-Calibration**: Continuously tracks ambient lighting bounds (`minLdr` and `maxLdr`) with safety margins, automatically adjusting to room lighting conditions without hardcoded thresholds.
- **Human Eye Gamma Correction**: The human eye perceives light logarithmically rather than linearly. The firmware computes a quadratic gamma curve:
  $$\text{PWM} = \frac{\text{intensity}^2 \times 4095}{1000^2}$$
  This prevents abrupt stepping and ensures ultra-smooth dimming and brightening transitions.

### Mode 1: Manual Potentiometer Dimming
- **Onboard Status LED (`PC13`)**: **ON**
- **Direct 12-bit Control**: Directly maps potentiometer rotation across 4,096 discrete duty cycle steps.
- **Zero-Point Deadband**: Values below 40 counts are clamped to 0 to guarantee that the LED completely turns off when the potentiometer is turned fully counter-clockwise.

---

## Build and Upload

1. Open [`Adaptive_LED_Dimmer.ino`](Adaptive_LED_Dimmer.ino) in the Arduino IDE (v2.x).
2. Ensure the **STM32duino** core is installed via the Boards Manager (**STM32 MCU based boards**).
3. Select board: `Generic STM32F1 series` -> `BluePill F103C8`.
4. Select upload method (e.g. `STLink` or `Serial / DFU`).
5. Compile and upload to the Blue Pill.
6. Press the button to switch modes, cover the LDR to observe night-light brightening, or switch to manual mode to dial the brightness via the potentiometer.
