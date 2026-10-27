# Pedestrian Crosswalk Signal Controller

An Arduino-style STM32 Blue Pill sketch for vehicle signals and pedestrian crossings. It uses a finite-state machine, pushbutton calls, timed walk and flashing phases, and an onboard heartbeat LED.

## Hardware and pinout

| Signal | STM32 pin |
| --- | --- |
| Vehicle 1 red / yellow / green | PA0 / PA1 / PA2 |
| Vehicle 2 red / yellow / green | PA3 / PA4 / PA5 |
| Pedestrian 1 red / green | PA6 / PA7 |
| Pedestrian 2 red / green | PB0 / PB1 |
| Pedestrian 1 pushbutton | PB12 |
| Pedestrian 2 pushbutton | PB13 |
| Onboard heartbeat LED | PC13 |

Buttons are active HIGH and require external 10 kOhm pull-down resistors. PC13 is treated as an active-LOW LED. Use current-limiting resistors for external LEDs and a common ground.

## Operation

The sequence is vehicle green, vehicle yellow, all-red clearance, pedestrian walk, flashing pedestrian warning, all-red clearance, vehicle yellow, then vehicle green. Both vehicle directions and both pedestrian signals are currently driven together. Up to two button press events are accepted per 30-second window.

| Setting | Duration |
| --- | ---: |
| Minimum vehicle green | 7 s |
| Vehicle yellow | 3 s |
| All-red clearance | 1.5 s |
| Pedestrian walk | 6 s |
| Flashing warning / toggle interval | 3 s / 300 ms |
| Vehicle-release yellow | 2 s |
| Heartbeat toggle | 500 ms |

## Build and upload

Install the Arduino IDE and a compatible STM32duino board package. Open `3ledway.ino`, select the matching STM32F103C8T6 board and upload method, then compile and upload.

## Safety

Educational prototype only; it is not a certified traffic controller. The current sketch can show green in both vehicle directions simultaneously, and both pedestrian signals can show walk together. Test with LEDs on a bench only; do not connect it to live traffic signals. A real installation requires independent safety interlocks and regulatory approval.