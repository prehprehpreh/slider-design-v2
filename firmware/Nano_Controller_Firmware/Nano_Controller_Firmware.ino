// ============================================================
// Nano/Uno Controller - Transmitter Firmware
// Sends joystick + button data to Mega via NRF24
// Menu mode for settings (Start toggles), saved to EEPROM
// ============================================================

#include <SPI.h>
#include <RF24.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

// --- NRF24 pins ---
#define CE_PIN  8
#define CSN_PIN 9
RF24 radio(CE_PIN, CSN_PIN);
const byte rfAddress[6] = "00001";

// --- OLED ---
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool oledOK = false;

// --- Joystick pins ---
#define L_JOY_X  A0
#define L_JOY_Y  A1
#define R_JOY_X  A2
#define R_JOY_Y  A3

// --- Button pins ---
#define BTN_START   2
#define BTN_X       3
#define BTN_Y       4
#define BTN_DPAD_L  5
#define BTN_DPAD_R  6
#define BTN_A       7
#define BTN_B       10

const uint8_t buttonPins[] = { BTN_START, BTN_X, BTN_Y, BTN_DPAD_L, BTN_DPAD_R, BTN_A, BTN_B };
const char* buttonLabels[] = { "START", "X", "Y", "D-L", "D-R", "A", "B" };
const uint8_t NUM_BUTTONS = 7;

// Button bitmasks
#define MASK_START  0x01
#define MASK_X      0x02
#define MASK_Y      0x04
#define MASK_DPAD_L 0x08
#define MASK_DPAD_R 0x10
#define MASK_A      0x20
#define MASK_B      0x40

// --- Compact binary packet ---
struct __attribute__((packed)) ControllerPacket {
    uint8_t magic;
    int8_t  lx;
    int8_t  ly;
    int8_t  rx;
    int8_t  ry;
    uint8_t buttons;
    uint8_t checksum;
};
ControllerPacket txPacket;

// --- Modes ---
enum Mode { MODE_NORMAL, MODE_MENU };
Mode currentMode = MODE_NORMAL;

// --- Settings (persisted in EEPROM) ---
struct Settings {
    uint8_t magic;
    uint8_t sensitivity;   // 0-100 (default 70)
    uint8_t invertMask;    // bit0=LX, bit1=LY, bit2=RX, bit3=RY
    uint8_t speedScale;    // 0-100 (default 100)
};
Settings settings;
#define EEPROM_ADDR 0
#define SETTINGS_MAGIC 0x42

// --- Menu ---
const char* menuItems[] = {
    "Sensitivity",
    "Invert LX",
    "Invert LY",
    "Invert RX",
    "Invert RY",
    "Speed Scale"
};
const uint8_t MENU_COUNT = 6;
uint8_t menuIndex = 0;

// --- Button state ---
uint8_t lastButtonMask = 0;
bool loopEnabled = false; // Y sets true, X sets false; queried by Mega after each preset

// --- Joystick calibration ---
int joyCenter[4];

// --- Timing ---
unsigned long lastTxTime = 0;
const unsigned long TX_INTERVAL = 50; // 20 Hz

// --- TX diagnostics ---
uint16_t txOkCount = 0;
uint16_t txFailCount = 0;

// ============================================================
void setup() {
    Serial.begin(115200);

    // Init buttons
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
        pinMode(buttonPins[i], INPUT_PULLUP);
    }

    // Init OLED
    Wire.begin();
    delay(100);
    oledOK = display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    if (oledOK) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println(F("Nano Controller"));
        display.display();
    } else {
        Serial.println(F("OLED not found"));
    }

    // Load settings from EEPROM
    loadSettings();

    // Calibrate joystick centers
    delay(200);
    joyCenter[0] = analogRead(L_JOY_X);
    joyCenter[1] = analogRead(L_JOY_Y);
    joyCenter[2] = analogRead(R_JOY_X);
    joyCenter[3] = analogRead(R_JOY_Y);

    // Init NRF24
    if (!radio.begin()) {
        Serial.println(F("NRF24 not found!"));
        if (oledOK) {
            display.println(F("NRF24 ERROR"));
            display.display();
        }
        while (1) {} // Halt
    }
    radio.setPALevel(RF24_PA_LOW);
    radio.setDataRate(RF24_250KBPS);
    radio.setChannel(76);
    radio.setAutoAck(false);
    radio.openWritingPipe(rfAddress);
    radio.stopListening();

    Serial.println(F("NRF24 TX ready"));
    if (oledOK) {
        display.println(F("NRF24 OK"));
        display.display();
        delay(1000);
    }
}

// ============================================================
void loop() {
    uint8_t currentMask = readButtonMask();

    // --- START button: toggle Normal / Menu ---
    bool startPressed = (currentMask & MASK_START) && !(lastButtonMask & MASK_START);
    if (startPressed) {
        if (currentMode == MODE_NORMAL) {
            enterMenu();
        } else {
            exitMenu();
        }
    }

    if (currentMode == MODE_NORMAL) {
        doNormalMode(currentMask);
    } else {
        doMenuMode(currentMask);
    }

    lastButtonMask = currentMask;
    delay(20); // ~50 Hz input poll
}

// ============================================================
// NORMAL MODE
// ============================================================
void doNormalMode(uint8_t btnMask) {
    // Track loop state on Uno (Y=ON, A=OFF)
    if ((btnMask & MASK_Y) && !(lastButtonMask & MASK_Y)) {
        loopEnabled = true;
        Serial.println(F("Loop enabled"));
    }
    if ((btnMask & MASK_A) && !(lastButtonMask & MASK_A)) {
        loopEnabled = false;
        Serial.println(F("Loop disabled"));
    }

    // Read joysticks
    int rawLX = analogRead(L_JOY_X) - joyCenter[0];
    int rawLY = analogRead(L_JOY_Y) - joyCenter[1];
    int rawRX = analogRead(R_JOY_X) - joyCenter[2];
    int rawRY = analogRead(R_JOY_Y) - joyCenter[3];

    // Apply sensitivity and invert
    int8_t lx = processAxis(rawLX, 0);
    int8_t ly = processAxis(rawLY, 1);
    int8_t rx = processAxis(rawRX, 2);
    int8_t ry = processAxis(rawRY, 3);

    // Build packet
    txPacket.magic = 0xAB;
    txPacket.lx = lx;
    txPacket.ly = ly;
    txPacket.rx = rx;
    txPacket.ry = ry;
    txPacket.buttons = (btnMask & ~MASK_START) | (loopEnabled ? 0x80 : 0x00); // bit 7 = loop state

    // XOR checksum (all bytes except checksum itself)
    uint8_t* p = (uint8_t*)&txPacket;
    txPacket.checksum = 0;
    for (uint8_t i = 0; i < sizeof(ControllerPacket) - 1; i++) {
        txPacket.checksum ^= p[i];
    }

    // Transmit at TX_INTERVAL
    unsigned long now = millis();
    if (now - lastTxTime >= TX_INTERVAL) {
        lastTxTime = now;
        bool ok = radio.write(&txPacket, sizeof(txPacket));
        if (ok) txOkCount++; else txFailCount++;

        // Serial debug
        Serial.print(F("TX:"));
        Serial.print(ok ? F("OK") : F("FAIL"));
        Serial.print(F(" OK:")); Serial.print(txOkCount);
        Serial.print(F(" FAIL:")); Serial.print(txFailCount);
        Serial.print(F(" LX:")); Serial.print(lx);
        Serial.print(F(" LY:")); Serial.print(ly);
        Serial.print(F(" RX:")); Serial.print(rx);
        Serial.print(F(" RY:")); Serial.print(ry);
        Serial.print(F(" BTN:")); Serial.print(btnMask, HEX);
        Serial.print(F(" LOOP:")); Serial.print(loopEnabled ? F("1") : F("0"));
        Serial.println();

        // Listen for query from Mega (longer window for reliable response)
        radio.startListening();
        delay(1); // let radio settle into RX mode
        unsigned long listenStart = millis();
        while (millis() - listenStart < 20) {
            if (radio.available()) {
                ControllerPacket query;
                radio.read(&query, sizeof(query));
                if (query.magic == 0xBA) {
                    // Build response with loop state in bit 7
                    radio.stopListening();
                    delay(1); // let radio settle into TX mode
                    ControllerPacket resp;
                    resp.magic = 0xAC; // distinct from regular packets (0xAB)
                    resp.lx = 0; resp.ly = 0; resp.rx = 0; resp.ry = 0;
                    resp.buttons = loopEnabled ? 0x80 : 0x00;
                    resp.checksum = resp.magic ^ resp.lx ^ resp.ly ^ resp.rx ^ resp.ry ^ resp.buttons;
                    radio.write(&resp, sizeof(resp));
                    Serial.println(F("Query resp sent"));
                    break;
                }
            }
        }
        radio.stopListening(); // back to TX mode for next iteration
        delay(1); // let radio settle into TX mode before next loop
    }

    // OLED
    if (oledOK) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println(F("NORMAL MODE"));
        display.print(F("LX:")); display.print(lx);
        display.print(F(" LY:")); display.println(ly);
        display.print(F("RX:")); display.print(rx);
        display.print(F(" RY:")); display.println(ry);
        display.print(F("BTN:"));
        for (uint8_t i = 1; i < NUM_BUTTONS; i++) {
            if (btnMask & (1 << i)) {
                display.print(buttonLabels[i]);
                display.print(F(" "));
            }
        }
        display.println();
        display.print(F("LOOP:"));
        display.println(loopEnabled ? F("ON") : F("OFF"));
        display.display();
    }
}

// ============================================================
// MENU MODE
// ============================================================
void enterMenu() {
    currentMode = MODE_MENU;
    menuIndex = 0;
    Serial.println(F("Entering MENU"));
}

void exitMenu() {
    currentMode = MODE_NORMAL;
    saveSettings();
    Serial.println(F("Exiting MENU, saved."));
}

void doMenuMode(uint8_t btnMask) {
    bool dpadL = (btnMask & MASK_DPAD_L) && !(lastButtonMask & MASK_DPAD_L);
    bool dpadR = (btnMask & MASK_DPAD_R) && !(lastButtonMask & MASK_DPAD_R);
    bool btnA  = (btnMask & MASK_A) && !(lastButtonMask & MASK_A);
    bool btnB  = (btnMask & MASK_B) && !(lastButtonMask & MASK_B);

    // Navigate items with X/Y (up/down)
    bool btnX  = (btnMask & MASK_X) && !(lastButtonMask & MASK_X);
    bool btnY  = (btnMask & MASK_Y) && !(lastButtonMask & MASK_Y);

    if (btnX) {
        if (menuIndex > 0) menuIndex--;
    }
    if (btnY) {
        if (menuIndex < MENU_COUNT - 1) menuIndex++;
    }

    // Adjust values with D-pad L/R
    if (dpadL) adjustSetting(menuIndex, -5);
    if (dpadR) adjustSetting(menuIndex, +5);

    // OLED
    if (oledOK) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println(F("MENU MODE"));
        display.println(F("X/Y=Nav L/R=Adj"));

        for (uint8_t i = 0; i < MENU_COUNT; i++) {
            if (i == menuIndex) display.print(F("> "));
            else display.print(F("  "));
            display.print(menuItems[i]);
            display.print(F(": "));
            display.println(getSettingValue(i));
        }
        display.display();
    }
}

// ============================================================
// SETTINGS HELPERS
// ============================================================
int16_t getSettingValue(uint8_t idx) {
    switch (idx) {
        case 0: return settings.sensitivity;
        case 1: return (settings.invertMask & 0x01) ? 1 : 0;
        case 2: return (settings.invertMask & 0x02) ? 1 : 0;
        case 3: return (settings.invertMask & 0x04) ? 1 : 0;
        case 4: return (settings.invertMask & 0x08) ? 1 : 0;
        case 5: return settings.speedScale;
    }
    return 0;
}

void adjustSetting(uint8_t idx, int8_t delta) {
    switch (idx) {
        case 0:
            settings.sensitivity = constrain(settings.sensitivity + delta, 0, 100);
            break;
        case 1:
            settings.invertMask ^= 0x01;
            break;
        case 2:
            settings.invertMask ^= 0x02;
            break;
        case 3:
            settings.invertMask ^= 0x04;
            break;
        case 4:
            settings.invertMask ^= 0x08;
            break;
        case 5:
            settings.speedScale = constrain(settings.speedScale + delta, 0, 100);
            break;
    }
}

void loadSettings() {
    EEPROM.get(EEPROM_ADDR, settings);
    if (settings.magic != SETTINGS_MAGIC) {
        // First boot: defaults
        settings.magic = SETTINGS_MAGIC;
        settings.sensitivity = 70;
        settings.invertMask = 0;
        settings.speedScale = 100;
        saveSettings();
        Serial.println(F("Settings initialized to defaults."));
    } else {
        Serial.println(F("Settings loaded from EEPROM."));
    }
}

void saveSettings() {
    EEPROM.put(EEPROM_ADDR, settings);
}

// ============================================================
// JOYSTICK / BUTTON HELPERS
// ============================================================
int8_t processAxis(int raw, uint8_t axisIndex) {
    // Map raw difference (-512 to 512) to -100 to 100
    int val = map(constrain(raw, -512, 512), -512, 512, -100, 100);

    // Apply deadzone
    if (abs(val) < 10) val = 0;

    // Apply sensitivity curve: x * (sens/100)
    val = (int)(val * ((float)settings.sensitivity / 100.0));
    val = constrain(val, -100, 100);

    // Apply invert
    if (settings.invertMask & (1 << axisIndex)) val = -val;

    return (int8_t)val;
}

uint8_t readButtonMask() {
    uint8_t mask = 0;
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
        if (digitalRead(buttonPins[i]) == LOW) {
            mask |= (1 << i);
        }
    }
    return mask;
}
