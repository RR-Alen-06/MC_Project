# RFID Attendance & Card Management System (RC522 + SSD1306 OLED)

A standalone, non-blocking RFID attendance and card provisioning system developed for the **STM32F103C8T6 (Blue Pill)** using the Arduino core for STM32 (STM32duino). The system uses an **MFRC522** SPI RFID transceiver to read and manage card UIDs, an **SSD1306 128×64 I2C OLED** display for real-time status monitoring, and dual status LEDs with an acoustic buzzer for user feedback—operating completely independently without requiring a Serial Monitor.

---

## Hardware Requirements

- **Microcontroller**: STM32F103C8T6 "Blue Pill" (ARM Cortex-M3 @ 72 MHz)
- **RFID Reader**: MFRC522 13.56 MHz RFID Module (SPI interface, 3.3V logic/power)
- **RFID Tags**: MIFARE Classic 1K / Ultralight / NTAG cards or keyfobs (1x Master Card + up to 30 user tags)
- **OLED Display**: 0.96" SSD1306 128×64 I2C OLED Display Module (`0x3C` address)
- **Visual Indicators**:
  - 1x Green LED (Access granted / Tag enrolled / Master confirmation)
  - 1x Red LED (Access denied / Tag removed / Hardware error)
  - 2x 220 Ω – 330 Ω current-limiting resistors
- **Acoustic Actuator**:
  - Active Buzzer driven via BC547 / 2N2222 NPN transistor with 1 kΩ base resistor (or low-current 3.3V active buzzer)
- **Prototyping**: Solderless breadboard and jumper wires

---

## Pinout and Hardware Wiring

| Peripheral | Module Pin | STM32 Pin | Bus / Function | Electrical Connection Notes |
| :--- | :--- | :--- | :--- | :--- |
| **RC522 RFID** | `SDA (SS)` | `PA4` | SPI1 NSS (Chip Select) | Digital output chip select (Active LOW). |
| **RC522 RFID** | `SCK` | `PA5` | SPI1 SCK | SPI1 Serial Clock. |
| **RC522 RFID** | `MISO` | `PA6` | SPI1 MISO | SPI1 Master In Slave Out. |
| **RC522 RFID** | `MOSI` | `PA7` | SPI1 MOSI | SPI1 Master Out Slave In. |
| **RC522 RFID** | `RST` | `PB0` | Digital Output | Hardware reset pin for RC522 transceiver. |
| **RC522 RFID** | `3.3V` / `GND` | `3.3V` / `GND` | Power Rail | **CRITICAL**: Supply RC522 strictly with `3.3V` (do not connect to 5V power). |
| **SSD1306 OLED** | `SCL` | `PB6` | I2C1 SCL | I2C clock running at 400 kHz Fast Mode. |
| **SSD1306 OLED** | `SDA` | `PB7` | I2C1 SDA | I2C data line. |
| **SSD1306 OLED** | `VCC` / `GND` | `3.3V` / `GND` | Power Rail | Powered from 3.3V rail and common ground. |
| **Green LED** | Anode (+) | `PA1` | Digital Output | Connect via 220–330 Ω series resistor to `PA1`; cathode to `GND`. |
| **Red LED** | Anode (+) | `PA2` | Digital Output | Connect via 220–330 Ω series resistor to `PA2`; cathode to `GND`. |
| **Buzzer** | Input / Base | `PA3` | Digital Output | Connect via 1 kΩ resistor to NPN transistor base switching the buzzer. |

---

## Operational Logic & Modes

### 1. Hardware Self-Test & Hot-Plug Recovery (`initRfid`)
- On startup, the firmware initializes the I2C OLED at 400 kHz and performs a hardware reset and SPI register probe (`MFRC522::VersionReg`) at 1 MHz before calling `PCD_Init()`.
- If the RC522 transceiver fails to respond (`0x00` or `0xFF`), the OLED displays `RFID ERROR: Reader not found` with 3 error beeps, and the main loop automatically attempts to re-initialize the reader every 2 seconds until communication is restored.

### 2. Master Card Provisioning
- **Dynamic First-Scan Master**: By default (`PRESET_MASTER` all `0x00`), the very first card tapped after boot is registered as the **Master Card** (confirmed by 2 beeps, a 1 s green LED flash, and `MASTER SET` on the OLED).
- **Hardcoded Master Option**: Setting `PRESET_MASTER[4]` in the sketch to a 4-byte UID locks in a permanent master card across power cycles.

### 3. Enrollment Mode (`MODE_ENROLL` — Card Registry Write/Delete)
- **Entering Enroll Mode**: Tap the **Master Card** while in Normal Mode. The OLED switches to the `ENROLL MODE` screen (`Tap NEW card = add`, `Tap OLD card = remove`, `Master card = exit`) with a 15-second inactivity timeout (`ENROLL_TIMEOUT`).
- **Adding a Card**: Tapping an unknown card stores its UID in the `tags[]` registry (up to `MAX_TAGS = 30`), emits 1 beep, flashes the Green LED, and displays `TAG ADDED (Tag #N)`.
- **Removing a Card**: Tapping an already-enrolled card removes it from `tags[]`, decrements `presentCount` if it was marked present, emits 2 beeps, flashes the Red LED, and displays `TAG REMOVED`.
- **Exiting Enroll Mode**: Tap the **Master Card** again or wait 15 seconds without activity to automatically return to Normal Mode.

### 4. Normal Attendance Mode (`MODE_NORMAL` — Card Read & Verification)
- **Idle Screen**: Displays `RFID ATTENDANCE - Please tap your card...` along with the live counter `Present: X / Y`.
- **First Check-In (`ATTENDANCE OK`)**: Tapping an enrolled card that is not yet marked present sets `present = true`, increments `presentCount`, triggers 1 beep and a 1.5 s Green LED flash, and shows `Marked PRESENT` on the OLED.
- **Duplicate Tap (`ALREADY MARKED`)**: Tapping an enrolled card that is already present triggers 2 short beeps, a 300 ms Green LED flash, and notifies `Already present`.
- **Unauthorized Card (`ACCESS DENIED`)**: Tapping an unknown card triggers 3 error beeps, a 1.5 s Red LED flash, and displays `Unknown card` along with its hexadecimal UID on the OLED.

---

## Required Libraries & Build Instructions

1. **Install Required Libraries** in Arduino IDE Library Manager:
   - `MFRC522` by GithubCommunity
   - `Adafruit SSD1306` by Adafruit
   - `Adafruit GFX Library` by Adafruit
2. Open [`RFID_Attendance_System.ino`](RFID_Attendance_System.ino) in the Arduino IDE (v2.x).
3. Ensure the **STM32 MCU based boards** core is installed via Boards Manager.
4. Select board configuration:
   - **Board**: `Generic STM32F1 series`
   - **Board part number**: `BluePill F103C8` (or `STM32F103C8Tx`)
   - **Upload method**: `STLink` (recommended) or `STM32CubeProgrammer (Serial)`
5. Compile and upload to the STM32 Blue Pill.

