/*
 * RFID Attendance - STM32 Blue Pill + RC522 + SSD1306 OLED
 * Enroll / remove cards using a MASTER card. No Serial Monitor needed.
 * Includes Self-Healing Auto-Pin-Scanner, MFRC522 Self-Test & Multi-Gain RF Sweep.
 *
 * Standard Wiring:
 * RC522: SDA/SS->PA4, SCK->PA5, MISO->PA6, MOSI->PA7, RST->PB0, 3.3V, GND
 * OLED : SCL->PB6, SDA->PB7, VCC 3.3V, GND
 * Green LED->PA1, Red LED->PA2 (each with 220-330R to GND)
 * Buzzer->PA3 (via BC547 transistor + 1k) or directly if low-current active buzzer
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ------------------------------ SETTINGS ------------------------------------
#define SCREEN_W 128
#define SCREEN_H 64
#define OLED_ADDR 0x3C
#define MAX_TAGS 30
#define ENROLL_TIMEOUT 15000UL // ms of inactivity before leaving enroll mode
#define MSG_TIME 1500UL        // how long result screens stay visible
#define SCAN_LOCKOUT 700UL     // ms ignored after each card read
#define HEALTH_CHECK_MS 1000UL // ms between RC522 antenna/bus health checks

// Optional: put your master card UID here so it survives resets.
// Leave all zeros to let the first scanned card become master.
const byte PRESET_MASTER[4] = {0x00, 0x00, 0x00, 0x00};

// ------------------------------ PINS ----------------------------------------
#ifdef LED_GREEN
#undef LED_GREEN
#endif
const int DEFAULT_SS   = PA4;
const int DEFAULT_SCK  = PA5;
const int DEFAULT_MISO = PA6;
const int DEFAULT_MOSI = PA7;
const int DEFAULT_RST  = PB0;

const int LED_GREEN = PA1;
const int LED_RED   = PA2;
const int BUZZER    = PA3;

// Active RC522 pins (auto-detected if swapped/moved)
int pinSS   = DEFAULT_SS;
int pinSCK  = DEFAULT_SCK;
int pinMISO = DEFAULT_MISO;
int pinMOSI = DEFAULT_MOSI;
int pinRST  = DEFAULT_RST;
const char *activeMapName = "STD";

// ------------------------------ OBJECTS -------------------------------------
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

// ------------------------------ RC522 REGISTERS & COMMANDS ------------------
#define REG_CommandReg      0x01
#define REG_ComIEnReg       0x02
#define REG_ComIrqReg       0x04
#define REG_DivIrqReg       0x05
#define REG_ErrorReg        0x06
#define REG_Status2Reg      0x08
#define REG_FIFODataReg     0x09
#define REG_FIFOLevelReg    0x0A
#define REG_ControlReg      0x0C
#define REG_BitFramingReg   0x0D
#define REG_CollReg         0x0E
#define REG_ModeReg         0x11
#define REG_TxModeReg       0x12
#define REG_RxModeReg       0x13
#define REG_TxControlReg    0x14
#define REG_TxASKReg        0x15
#define REG_RxThresholdReg  0x18
#define REG_DemodReg        0x19
#define REG_GsNReg          0x27
#define REG_CWGsPReg        0x28
#define REG_ModGsPReg       0x29
#define REG_CRCResultRegH   0x21
#define REG_CRCResultRegL   0x22
#define REG_ModWidthReg     0x24
#define REG_RFCfgReg        0x26
#define REG_TModeReg        0x2A
#define REG_TPrescalerReg   0x2B
#define REG_TReloadRegH     0x2C
#define REG_TReloadRegL     0x2D
#define REG_AutoTestReg     0x36
#define REG_VersionReg      0x37

#define PCD_Idle            0x00
#define PCD_Mem             0x01
#define PCD_CalcCRC         0x03
#define PCD_Transceive      0x0C
#define PCD_SoftReset       0x0F

#define PICC_REQIDL         0x26
#define PICC_WUPA           0x52
#define PICC_ANTICOLL_CL1   0x93
#define PICC_ANTICOLL_CL2   0x95
#define PICC_SELECT_CL1     0x93
#define PICC_HLTA           0x50

// ------------------------------ DATA ----------------------------------------
struct Tag {
  byte uid[10];
  byte len;
  bool present;
};

Tag tags[MAX_TAGS];
int tagCount = 0;
int presentCount = 0;

byte masterUid[10];
byte masterLen = 0;

byte currentUid[10];
byte currentUidLen = 0;

enum Mode { MODE_NORMAL, MODE_ENROLL };
Mode mode = MODE_NORMAL;

bool oledOk = false;
bool rfidOk = false;
bool selfTestOk = false;
byte rfidVersion = 0x00;
byte rfidTxCtrl = 0x00;
byte lastIrq = 0x00;
byte lastErr = 0x00;
byte gainStep = 0;
unsigned int pollCounter = 0;
unsigned int atqaHits = 0;

// Gain & RxThreshold sweep table for clone/genuine RC522 modules:
// Cycles through 48dB, 43dB, 38dB, 33dB, and sensitive RxThreshold profiles
const byte GAIN_TABLE[6] = { 0x70, 0x60, 0x50, 0x40, 0x70, 0x50 };
const byte RXTH_TABLE[6] = { 0x84, 0x84, 0x84, 0x84, 0x55, 0x55 };

// Diagnostic state for PA4, PA5, PA6, PA7, PB0
char diagLine1[26];
char diagLine2[26];
char diagLine3[26];

// timing state (all non-blocking)
unsigned long enrollEnd = 0;
unsigned long lockUntil = 0;
unsigned long msgUntil = 0;
bool msgActive = false;
unsigned long greenUntil = 0;
unsigned long redUntil = 0;
unsigned long lastRfidTry = 0;
unsigned long lastHealthCheck = 0;

// buzzer state
byte beepLeft = 0;
unsigned int beepOnMs = 0, beepOffMs = 0;
bool beepState = false;
unsigned long beepNext = 0;

// ============================== SOFT SPI RC522 ENGINE =======================
// Works reliably on any pin mapping, immune to STM32 hardware SPI locks

byte softSpiTransfer(byte data) {
  byte rx = 0;
  for (byte i = 0; i < 8; i++) {
    digitalWrite(pinMOSI, (data & 0x80) ? HIGH : LOW);
    data <<= 1;
    delayMicroseconds(2);
    digitalWrite(pinSCK, HIGH);
    delayMicroseconds(2);
    rx <<= 1;
    if (digitalRead(pinMISO)) rx |= 1;
    digitalWrite(pinSCK, LOW);
    delayMicroseconds(2);
  }
  return rx;
}

void rc522WriteReg(byte reg, byte val) {
  digitalWrite(pinSS, LOW);
  softSpiTransfer((reg << 1) & 0x7E);
  softSpiTransfer(val);
  digitalWrite(pinSS, HIGH);
}

byte rc522ReadReg(byte reg) {
  digitalWrite(pinSS, LOW);
  softSpiTransfer(((reg << 1) & 0x7E) | 0x80);
  byte val = softSpiTransfer(0x00);
  digitalWrite(pinSS, HIGH);
  return val;
}

void rc522SetBitMask(byte reg, byte mask) {
  rc522WriteReg(reg, rc522ReadReg(reg) | mask);
}

void rc522ClearBitMask(byte reg, byte mask) {
  rc522WriteReg(reg, rc522ReadReg(reg) & (~mask));
}

bool isValidVersion(byte v) {
  return (v == 0x90 || v == 0x91 || v == 0x92 || v == 0x82 ||
          v == 0x88 || v == 0xB2 || v == 0x12 || v == 0x18);
}

// Run internal MFRC522 BIST / FIFO / Digital Self-Test (Datasheet section 16.1.1)
bool runRc522SelfTest() {
  rc522WriteReg(REG_CommandReg, PCD_SoftReset);
  delay(15);
  rc522WriteReg(REG_FIFOLevelReg, 0x80);
  for (byte i = 0; i < 25; i++) rc522WriteReg(REG_FIFODataReg, 0x00);
  rc522WriteReg(REG_CommandReg, PCD_Mem);
  delay(5);
  rc522WriteReg(REG_AutoTestReg, 0x09);
  rc522WriteReg(REG_FIFODataReg, 0x00);
  rc522WriteReg(REG_CommandReg, PCD_CalcCRC);

  unsigned long t0 = millis();
  byte fifoLen = 0;
  while (millis() - t0 < 50) {
    fifoLen = rc522ReadReg(REG_FIFOLevelReg);
    if (fifoLen >= 64) break;
  }
  rc522WriteReg(REG_CommandReg, PCD_Idle);
  rc522WriteReg(REG_AutoTestReg, 0x00); // MUST clear AutoTestReg back to 0x00 for normal RF!

  // Verify the chip produced 64 non-zero BIST bytes in the FIFO
  byte nonZero = 0;
  for (byte i = 0; i < 8; i++) {
    if (rc522ReadReg(REG_FIFODataReg) != 0x00) nonZero++;
  }
  return (fifoLen >= 64 && nonZero > 0);
}

void configureRfidRegisters(bool doSoftReset) {
  if (doSoftReset) {
    rc522WriteReg(REG_CommandReg, PCD_SoftReset);
    unsigned long t0 = millis();
    while (millis() - t0 < 50) {
      if ((rc522ReadReg(REG_CommandReg) & 0x10) == 0) break;
      delay(2);
    }
    delay(10);
  }

  // Ensure normal analog/digital mode (disable any leftover AutoTest mode)
  rc522WriteReg(REG_AutoTestReg, 0x00);
  rc522WriteReg(REG_CommandReg, PCD_Idle);

  // Reset 106 kBd framing & CRC
  rc522WriteReg(REG_TxModeReg, 0x00);
  rc522WriteReg(REG_RxModeReg, 0x00);
  rc522WriteReg(REG_ModWidthReg, 0x26);

  // Configure Timer
  rc522WriteReg(REG_TModeReg, 0x80);
  rc522WriteReg(REG_TPrescalerReg, 0xA9);
  rc522WriteReg(REG_TReloadRegH, 0x03);
  rc522WriteReg(REG_TReloadRegL, 0xE8);

  // 100% ASK modulation & CRC preset 0x6363
  rc522WriteReg(REG_TxASKReg, 0x40);
  rc522WriteReg(REG_ModeReg, 0x3D);

  // Boost CW & Modulated TX Output Conductance to maximum (0x3F) for strong 13.56 MHz field on 3.3V!
  rc522WriteReg(REG_GsNReg, 0xF8);
  rc522WriteReg(REG_CWGsPReg, 0x3F);
  rc522WriteReg(REG_ModGsPReg, 0x3F);

  // Apply current Gain & RxThreshold profile
  rc522WriteReg(REG_RFCfgReg, GAIN_TABLE[gainStep % 6]);
  rc522WriteReg(REG_RxThresholdReg, RXTH_TABLE[gainStep % 6]);
  rc522WriteReg(REG_DemodReg, 0x4D);

  // Turn Antenna ON (TX1 & TX2 = bits 0..1)
  byte tx = rc522ReadReg(REG_TxControlReg);
  if ((tx & 0x03) != 0x03) {
    rc522WriteReg(REG_TxControlReg, tx | 0x03);
  }
  delay(4);
}

bool rc522CalculateCRC(const byte *data, byte len, byte *result) {
  rc522WriteReg(REG_CommandReg, PCD_Idle);
  rc522WriteReg(REG_DivIrqReg, 0x04);
  rc522WriteReg(REG_FIFOLevelReg, 0x80);
  for (byte i = 0; i < len; i++) {
    rc522WriteReg(REG_FIFODataReg, data[i]);
  }
  rc522WriteReg(REG_CommandReg, PCD_CalcCRC);
  unsigned long t0 = millis();
  while (millis() - t0 < 40) {
    byte n = rc522ReadReg(REG_DivIrqReg);
    if (n & 0x04) {
      rc522WriteReg(REG_CommandReg, PCD_Idle);
      result[0] = rc522ReadReg(REG_CRCResultRegL);
      result[1] = rc522ReadReg(REG_CRCResultRegH);
      return true;
    }
  }
  return false;
}

bool rc522Transceive(const byte *sendData, byte sendLen, byte *backData, byte *backBits, byte validBits) {
  rc522WriteReg(REG_CommandReg, PCD_Idle);
  rc522WriteReg(REG_ComIEnReg, 0x77 | 0x80);
  rc522ClearBitMask(REG_ComIrqReg, 0x80);
  rc522WriteReg(REG_ComIrqReg, 0x7F);
  rc522WriteReg(REG_FIFOLevelReg, 0x80);
  for (byte i = 0; i < sendLen; i++) {
    rc522WriteReg(REG_FIFODataReg, sendData[i]);
  }
  rc522WriteReg(REG_BitFramingReg, validBits & 0x07);
  rc522WriteReg(REG_CommandReg, PCD_Transceive);
  rc522SetBitMask(REG_BitFramingReg, 0x80); // StartSend

  unsigned long t0 = millis();
  bool completed = false;
  byte irq = 0;
  while (millis() - t0 < 35) {
    irq = rc522ReadReg(REG_ComIrqReg);
    if (irq & 0x30) { // RxIRq (0x20) or IdleIRq (0x10)
      completed = true;
      break;
    }
    if (irq & 0x01) { // TimerIRq
      break;
    }
  }
  rc522ClearBitMask(REG_BitFramingReg, 0x80);
  lastIrq = irq;
  lastErr = rc522ReadReg(REG_ErrorReg);

  if (!completed) return false;
  if (lastErr & 0x13) return false; // BufferOvfl, ParityErr, ProtocolErr

  if (backData && backBits) {
    byte n = rc522ReadReg(REG_FIFOLevelReg);
    if (n == 0) return false;
    if (n > 16) n = 16;
    for (byte i = 0; i < n; i++) {
      backData[i] = rc522ReadReg(REG_FIFODataReg);
    }
    byte lastBits = rc522ReadReg(REG_ControlReg) & 0x07;
    if (lastBits) *backBits = (n - 1) * 8 + lastBits;
    else *backBits = n * 8;
  }
  return true;
}

bool rc522RequestCard() {
  // Sweep across all 6 Gain/Threshold combinations on each scan so whether the card is
  // touching the coil (needs lower gain to avoid saturation) or 2-3cm away (needs 48dB), it locks on!
  for (byte g = 0; g < 6; g++) {
    gainStep = g;
    rc522WriteReg(REG_RFCfgReg, GAIN_TABLE[g]);
    rc522WriteReg(REG_RxThresholdReg, RXTH_TABLE[g]);
    rc522WriteReg(REG_TxModeReg, 0x00);
    rc522WriteReg(REG_RxModeReg, 0x00);
    rc522WriteReg(REG_ErrorReg, 0x00);
    rc522ClearBitMask(REG_Status2Reg, 0x08);
    rc522ClearBitMask(REG_CollReg, 0x80);

    byte cmd = PICC_WUPA;
    byte back[4] = {0};
    byte bits = 0;
    if (rc522Transceive(&cmd, 1, back, &bits, 7) && bits == 16) {
      atqaHits++;
      return true;
    }
    cmd = PICC_REQIDL;
    bits = 0;
    if (rc522Transceive(&cmd, 1, back, &bits, 7) && bits == 16) {
      atqaHits++;
      return true;
    }
  }
  return false;
}

bool rc522ReadCardUid() {
  rc522WriteReg(REG_BitFramingReg, 0x00);
  rc522ClearBitMask(REG_CollReg, 0x80);
  byte cmd[2] = { PICC_ANTICOLL_CL1, 0x20 };
  byte back[10] = {0};
  byte bits = 0;
  if (!rc522Transceive(cmd, 2, back, &bits, 0)) return false;
  if (bits != 40) return false;
  if ((back[0] ^ back[1] ^ back[2] ^ back[3]) != back[4]) return false;

  if (back[0] == 0x88) {
    currentUid[0] = back[1];
    currentUid[1] = back[2];
    currentUid[2] = back[3];

    byte sel1[9] = { PICC_SELECT_CL1, 0x70, back[0], back[1], back[2], back[3], back[4], 0, 0 };
    if (rc522CalculateCRC(sel1, 7, &sel1[7])) {
      byte sak[4];
      byte sakBits = 0;
      if (rc522Transceive(sel1, 9, sak, &sakBits, 0)) {
        byte cmd2[2] = { PICC_ANTICOLL_CL2, 0x20 };
        byte back2[10] = {0};
        byte bits2 = 0;
        if (rc522Transceive(cmd2, 2, back2, &bits2, 0) && bits2 == 40) {
          currentUid[3] = back2[0];
          currentUid[4] = back2[1];
          currentUid[5] = back2[2];
          currentUid[6] = back2[3];
          currentUidLen = 7;
          return true;
        }
      }
    }
  }

  for (byte i = 0; i < 4; i++) currentUid[i] = back[i];
  currentUidLen = 4;
  return true;
}

void rc522HaltCard() {
  byte cmd[4] = { PICC_HLTA, 0x00, 0x00, 0x00 };
  if (rc522CalculateCRC(cmd, 2, &cmd[2])) {
    byte dummy[4];
    byte bits = 0;
    rc522Transceive(cmd, 4, dummy, &bits, 0);
  }
  rc522ClearBitMask(REG_Status2Reg, 0x08);
}

// ============================== PIN AUTO-SCANNER ============================

struct PinMapCandidate {
  int ss, sck, miso, mosi, rst;
  const char *name;
};

const PinMapCandidate CANDIDATES[] = {
  { PA4, PA5, PA6, PA7, PB0, "STD" },
  { PA4, PA5, PA7, PA6, PB0, "SWP-MI" },
  { PA4, PA5, PA6, PA7, PB1, "RST-B1" },
  { PA4, PA5, PA6, PA7, PA0, "RST-A0" },
  { PA4, PA5, PA6, PA7, PA8, "RST-A8" },
  { PA4, PA5, PA6, PA7, PB10, "RST-B10" },
  { PB0, PA5, PA6, PA7, PA4, "SWP-SS" },
  { PB12, PB13, PB14, PB15, PB0, "SPI2" },
  { PB12, PB13, PB14, PB15, PA8, "SPI2A8" },
  { PB0, PA7, PA6, PA5, PA4, "REV-HD" }
};
const int NUM_CANDIDATES = sizeof(CANDIDATES) / sizeof(CANDIDATES[0]);

char pinStateChar(int pin) {
  pinMode(pin, INPUT_PULLUP);
  delayMicroseconds(50);
  int hi = digitalRead(pin);
  pinMode(pin, INPUT_PULLDOWN);
  delayMicroseconds(50);
  int lo = digitalRead(pin);
  pinMode(pin, INPUT);
  if (hi == LOW)  return '0';
  if (lo == HIGH) return '1';
  return 'Z';
}

void runElectricalDiagnostics(byte rawVer) {
  char s4 = pinStateChar(PA4);
  char s5 = pinStateChar(PA5);
  char s6 = pinStateChar(PA6);
  char s7 = pinStateChar(PA7);
  char sB = pinStateChar(PB0);
  snprintf(diagLine1, sizeof(diagLine1), "4:%c 5:%c 6:%c 7:%c B0:%c", s4, s5, s6, s7, sB);
  if (s6 == 'Z' && s7 == 'Z' && sB == 'Z') {
    snprintf(diagLine2, sizeof(diagLine2), "All pins floating(Z)");
    snprintf(diagLine3, sizeof(diagLine3), "Check 3.3V/GND/Solder");
  } else if (s6 == '0') {
    snprintf(diagLine2, sizeof(diagLine2), "PA6(MISO) stuck LOW");
    snprintf(diagLine3, sizeof(diagLine3), "Check PA6 wire/power");
  } else {
    snprintf(diagLine2, sizeof(diagLine2), "RawV:0x%02X No RC522", rawVer);
    snprintf(diagLine3, sizeof(diagLine3), "Check RC522 solder/3V3");
  }
}

bool tryCandidate(int idx) {
  const PinMapCandidate &c = CANDIDATES[idx];
  pinSS   = c.ss;
  pinSCK  = c.sck;
  pinMISO = c.miso;
  pinMOSI = c.mosi;
  pinRST  = c.rst;

  pinMode(pinRST, OUTPUT);
  digitalWrite(pinRST, LOW);
  delay(10);
  digitalWrite(pinRST, HIGH);
  delay(20);

  pinMode(pinSS, OUTPUT);
  digitalWrite(pinSS, HIGH);
  pinMode(pinSCK, OUTPUT);
  digitalWrite(pinSCK, LOW);
  pinMode(pinMOSI, OUTPUT);
  digitalWrite(pinMOSI, LOW);
  pinMode(pinMISO, INPUT_PULLUP);

  rc522WriteReg(REG_TReloadRegL, 0xAA);
  byte rb1 = rc522ReadReg(REG_TReloadRegL);
  rc522WriteReg(REG_TReloadRegL, 0x55);
  byte rb2 = rc522ReadReg(REG_TReloadRegL);

  byte v1 = rc522ReadReg(REG_VersionReg);

  if ((rb1 == 0xAA && rb2 == 0x55) || isValidVersion(v1)) {
    activeMapName = c.name;
    selfTestOk = runRc522SelfTest();
    configureRfidRegisters(true);
    rfidVersion = rc522ReadReg(REG_VersionReg);
    rfidTxCtrl  = rc522ReadReg(REG_TxControlReg);
    if ((rfidTxCtrl & 0x03) == 0x03) {
      return true;
    }
  }
  return false;
}

bool initRfid() {
  for (int i = 0; i < NUM_CANDIDATES; i++) {
    if (tryCandidate(i)) {
      return true;
    }
  }
  pinSS   = DEFAULT_SS;
  pinSCK  = DEFAULT_SCK;
  pinMISO = DEFAULT_MISO;
  pinMOSI = DEFAULT_MOSI;
  pinRST  = DEFAULT_RST;
  pinMode(pinSS, OUTPUT); digitalWrite(pinSS, HIGH);
  pinMode(pinSCK, OUTPUT); digitalWrite(pinSCK, LOW);
  pinMode(pinMOSI, OUTPUT); digitalWrite(pinMOSI, LOW);
  pinMode(pinMISO, INPUT_PULLUP);
  byte rawV = rc522ReadReg(REG_VersionReg);
  rfidVersion = 0x00;
  rfidTxCtrl  = 0x00;
  runElectricalDiagnostics(rawV);
  return false;
}

// ============================== HELPERS =====================================

void uidToStr(const byte *u, byte n, char *out) {
  const char *hex = "0123456789ABCDEF";
  int p = 0;
  for (byte i = 0; i < n; i++) {
    out[p++] = hex[u[i] >> 4];
    out[p++] = hex[u[i] & 0x0F];
    if (i < n - 1) out[p++] = ' ';
  }
  out[p] = '\0';
}

bool sameUid(const byte *a, byte alen, const byte *b, byte blen) {
  if (alen != blen) return false;
  for (byte i = 0; i < alen; i++) if (a[i] != b[i]) return false;
  return true;
}

int findTag(const byte *u, byte n) {
  for (int i = 0; i < tagCount; i++)
    if (sameUid(tags[i].uid, tags[i].len, u, n)) return i;
  return -1;
}

// ---- Buzzer (non-blocking) ----
void startBeep(byte count, unsigned int onMs, unsigned int offMs) {
  beepLeft = count; beepOnMs = onMs; beepOffMs = offMs;
  beepState = false; beepNext = millis();
}

void updateBeep() {
  if (beepLeft == 0 && !beepState) return;
  if ((long)(millis() - beepNext) < 0) return;
  if (!beepState) {
    digitalWrite(BUZZER, HIGH);
    beepState = true;
    beepNext = millis() + beepOnMs;
  } else {
    digitalWrite(BUZZER, LOW);
    beepState = false;
    if (beepLeft > 0) beepLeft--;
    beepNext = millis() + beepOffMs;
  }
}

// ---- LEDs (non-blocking) ----
void flashGreen(unsigned long ms) { digitalWrite(LED_GREEN, HIGH); greenUntil = millis() + ms; }
void flashRed(unsigned long ms) { digitalWrite(LED_RED, HIGH); redUntil = millis() + ms; }

void updateLeds() {
  if (greenUntil && (long)(millis() - greenUntil) >= 0) { digitalWrite(LED_GREEN, LOW); greenUntil = 0; }
  if (redUntil && (long)(millis() - redUntil) >= 0) { digitalWrite(LED_RED, LOW); redUntil = 0; }
}

// ============================== OLED ========================================

void header(const char *title) {
  display.clearDisplay();
  display.fillRect(0, 0, SCREEN_W, 14, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  int w = strlen(title) * 6;
  display.setCursor((SCREEN_W - w) / 2, 3);
  display.print(title);
  display.setTextColor(SSD1306_WHITE);
}

void line(int y, const char *text) {
  display.setCursor(0, y);
  display.print(text);
}

void drawBase() {
  if (!oledOk) return;
  char buf[26];
  if (mode == MODE_ENROLL) {
    header("ENROLL MODE");
    line(18, "Tap NEW card = add");
    line(28, "Tap OLD card = remove");
    line(38, "Master card = exit");
    snprintf(buf, sizeof(buf), "Tags:%d/%d V:%02X A:%s",
             tagCount, MAX_TAGS, rfidVersion,
             ((rfidTxCtrl & 0x03) == 0x03) ? "ON" : "OF");
    line(54, buf);
  } else {
    header("RFID ATTENDANCE");
    if (masterLen == 0) {
      line(17, "Tap card = set MASTER");
    } else {
      line(17, "Please tap your card");
    }
    snprintf(buf, sizeof(buf), "V:%02X Tx:%02X BIST:%s",
             rfidVersion, rfidTxCtrl, selfTestOk ? "OK" : "ER");
    line(28, buf);
    snprintf(buf, sizeof(buf), "Irq:%02X Atq:%u %s %c",
             lastIrq, atqaHits, activeMapName, (pollCounter & 1) ? '*' : '.');
    line(39, buf);
    display.drawLine(0, 50, SCREEN_W, 50, SSD1306_WHITE);
    snprintf(buf, sizeof(buf), "Present: %d / %d", presentCount, tagCount);
    line(54, buf);
  }
  display.display();
}

void showMsg(const char *title, const char *l1, const char *l2, const char *l3) {
  msgActive = true;
  msgUntil = millis() + MSG_TIME;
  if (!oledOk) return;
  header(title);
  if (l1) line(20, l1);
  if (l2) line(33, l2);
  if (l3) line(46, l3);
  display.display();
}

void updateScreen() {
  if (mode == MODE_ENROLL && (long)(millis() - enrollEnd) >= 0) {
    mode = MODE_NORMAL;
    startBeep(2, 80, 80);
    showMsg("ENROLL ENDED", "Timed out.", "Back to normal mode", nullptr);
    return;
  }
  if (msgActive && (long)(millis() - msgUntil) >= 0) {
    msgActive = false;
    drawBase();
  }
}

// ============================== CARD LOGIC ==================================

void removeTag(int idx) {
  if (tags[idx].present) presentCount--;
  for (int i = idx; i < tagCount - 1; i++) tags[i] = tags[i + 1];
  tagCount--;
}

void handleCard() {
  byte *u = currentUid;
  byte n = currentUidLen;
  char uidStr[32];
  char buf[24];
  uidToStr(u, n, uidStr);

  // 1) No master yet -> first card becomes master
  if (masterLen == 0) {
    memcpy(masterUid, u, n);
    masterLen = n;
    startBeep(2, 120, 80);
    flashGreen(1000);
    showMsg("MASTER SET", "This card is master:", uidStr, "Tap it to enroll");
    return;
  }

  // 2) Master card -> toggle enroll mode
  if (sameUid(u, n, masterUid, masterLen)) {
    if (mode == MODE_NORMAL) {
      mode = MODE_ENROLL;
      enrollEnd = millis() + ENROLL_TIMEOUT;
      startBeep(1, 200, 0);
      flashGreen(500);
      showMsg("ENROLL MODE", "Master accepted", "Tap card to add/remove", nullptr);
    } else {
      mode = MODE_NORMAL;
      startBeep(2, 80, 80);
      showMsg("ENROLL ENDED", "Back to normal mode", nullptr, nullptr);
    }
    return;
  }

  int idx = findTag(u, n);

  // 3) Enroll mode: add or remove
  if (mode == MODE_ENROLL) {
    enrollEnd = millis() + ENROLL_TIMEOUT;
    if (idx >= 0) {
      removeTag(idx);
      startBeep(2, 100, 100);
      flashRed(600);
      showMsg("TAG REMOVED", uidStr, nullptr, nullptr);
    } else if (tagCount >= MAX_TAGS) {
      startBeep(3, 100, 100);
      flashRed(800);
      showMsg("MEMORY FULL", "Remove a card first", nullptr, nullptr);
    } else {
      memcpy(tags[tagCount].uid, u, n);
      tags[tagCount].len = n;
      tags[tagCount].present = false;
      tagCount++;
      startBeep(1, 150, 0);
      flashGreen(600);
      snprintf(buf, sizeof(buf), "Tag #%d added", tagCount);
      showMsg("TAG ADDED", buf, uidStr, nullptr);
    }
    return;
  }

  // 4) Normal mode: attendance
  if (idx >= 0) {
    snprintf(buf, sizeof(buf), "Tag #%d", idx + 1);
    if (!tags[idx].present) {
      tags[idx].present = true;
      presentCount++;
      startBeep(1, 150, 0);
      flashGreen(MSG_TIME);
      char cnt[24];
      snprintf(cnt, sizeof(cnt), "Present: %d / %d", presentCount, tagCount);
      showMsg("ATTENDANCE OK", buf, "Marked PRESENT", cnt);
    } else {
      startBeep(2, 70, 70);
      flashGreen(300);
      showMsg("ALREADY MARKED", buf, "Already present", nullptr);
    }
  } else {
    startBeep(3, 100, 100);
    flashRed(MSG_TIME);
    showMsg("ACCESS DENIED", "Unknown card", uidStr, nullptr);
  }
}

// ============================== SETUP / LOOP ================================

void checkRfidHealth() {
  if (millis() - lastHealthCheck < HEALTH_CHECK_MS) return;
  lastHealthCheck = millis();
  pollCounter++;

  rfidVersion = rc522ReadReg(REG_VersionReg);
  rfidTxCtrl  = rc522ReadReg(REG_TxControlReg);

  rc522WriteReg(REG_TReloadRegL, 0xE8);
  byte rb = rc522ReadReg(REG_TReloadRegL);

  if (rb != 0xE8 && !isValidVersion(rfidVersion)) {
    rfidOk = false;
    runElectricalDiagnostics(rfidVersion);
    showMsg("RFID LOST", diagLine1, diagLine2, diagLine3);
    return;
  }

  if ((rfidTxCtrl & 0x03) != 0x03) {
    configureRfidRegisters(false);
    rfidTxCtrl = rc522ReadReg(REG_TxControlReg);
  }

  if (!msgActive) {
    drawBase();
  }
}

void setup() {
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(BUZZER, LOW);

  bool presetUsed = false;
  for (byte i = 0; i < 4; i++) if (PRESET_MASTER[i] != 0) presetUsed = true;
  if (presetUsed) {
    memcpy(masterUid, PRESET_MASTER, 4);
    masterLen = 4;
  }

  Wire.begin();
  Wire.setClock(400000);
  oledOk = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  if (oledOk) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(20, 20);
    display.print(F("RFID ATTENDANCE"));
    display.setCursor(22, 36);
    display.print(F("Scanning RC522..."));
    display.display();
  }

  rfidOk = initRfid();

  if (!rfidOk) {
    startBeep(3, 200, 100);
    flashRed(2000);
    showMsg("RFID PIN DIAG", diagLine1, diagLine2, diagLine3);
  } else {
    startBeep(2, 80, 80);
    char verBuf[24];
    snprintf(verBuf, sizeof(verBuf), "V:%02X BIST:%s %s",
             rfidVersion, selfTestOk ? "PASS" : "FAIL", activeMapName);
    if (masterLen == 0) {
      showMsg("READY", verBuf, "Tap a card to set", "it as MASTER card");
      msgUntil = millis() + 3000;
    } else {
      drawBase();
    }
  }
}

void loop() {
  updateBeep();
  updateLeds();
  updateScreen();

  if (!rfidOk) {
    if (millis() - lastRfidTry > 1500) {
      lastRfidTry = millis();
      rfidOk = initRfid();
      if (rfidOk) {
        startBeep(2, 80, 80);
        char verBuf[24];
        snprintf(verBuf, sizeof(verBuf), "V:%02X BIST:%s %s",
                 rfidVersion, selfTestOk ? "PASS" : "FAIL", activeMapName);
        showMsg("RFID FOUND!", verBuf, "Tap card to set", "MASTER card");
      } else {
        showMsg("RFID PIN DIAG", diagLine1, diagLine2, diagLine3);
      }
    }
    return;
  }

  checkRfidHealth();

  if ((long)(millis() - lockUntil) < 0) return;

  if (!rc522RequestCard()) return;
  if (!rc522ReadCardUid()) return;

  handleCard();

  rc522HaltCard();
  lockUntil = millis() + SCAN_LOCKOUT;
}
