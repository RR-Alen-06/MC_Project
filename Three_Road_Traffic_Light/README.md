# Three-Road Traffic Light Controller

An Arduino-style STM32 Blue Pill sketch that cycles traffic signals for three roads. Each road receives a yellow transition, a five-second green phase, and a yellow clearance before the next road is activated.

## Hardware and pinout

| Signal | STM32 pin |
| --- | --- |
| Road 1 red / yellow / green | PA0 / PA1 / PA2 |
| Road 2 red / yellow / green | PA3 / PA4 / PA5 |
| Road 3 red / yellow / green | PA6 / PA7 / PB0 |

Use one current-limiting resistor per LED and connect all grounds together. Confirm the LED polarity and pin mapping before powering the circuit.

## Sequence and timing

| Phase | Duration |
| --- | ---: |
| All roads red at start of cycle | 3 s |
| Each road's initial yellow | 2 s |
| Each road's green | 5 s |
| Each road's yellow clearance | 2 s |
| Delay between roads | 1 s |

The order is Road 1, Road 2, then Road 3, after which the cycle repeats. The sketch uses blocking `delay()` calls.

## Build and upload

Open `Three_Road_Traffic_Light.ino` in the Arduino IDE and select the STM32F103C8T6 Blue Pill board using the STM32duino core. Compile and upload using a supported method such as ST-Link or Serial.

## Safety

This is an educational bench prototype, not a certified traffic controller. Test only with LEDs on a low-voltage breadboard; do not connect it to real traffic signals or use it to control vehicles.
