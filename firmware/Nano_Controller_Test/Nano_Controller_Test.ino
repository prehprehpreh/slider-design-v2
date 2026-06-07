// ============================================================
// Nano/Uno Controller - Hardware Test Sketch
// Tests all components: joysticks, buttons, OLED
// NRF24 is NOT tested here - just inputs and display
// ============================================================

#include <Wire.h>
#include <SPI.h>
#include <RF24.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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

const int BUTTON_PINS[] = { BTN_START, BTN_X, BTN_Y, BTN_DPAD_L, BTN_DPAD_R, BTN_A, BTN_B };
const char* BUTTON_NAMES[] = { "START", "X", "Y", "DPAD_L", "DPAD_R", "A", "B" };
const int NUM_BUTTONS = 7;

// --- NRF24 TX ---
#define CE_PIN  8
#define CSN_PIN 9
RF24 radio(CE_PIN, CSN_PIN);
const byte rfAddress[6] = "00001";

// --- Button state tracking (for edge detection) ---
bool lastButtonState[7] = { HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH };

// --- Joystick center calibration ---
int lxCenter, lyCenter, rxCenter, ryCenter;
bool oledOK = false;

// --- TX stats ---
unsigned long txCounter = 0;
uint16_t txOk = 0;
uint16_t txFail = 0;
const unsigned long TX_INTERVAL = 50;
unsigned long lastTxTime = 0;

void setup() {
    Serial.begin(115200);
    Serial.println(F("=== Controller Hardware Test ==="));

    // Button pins with internal pullup
    for (int i = 0; i < NUM_BUTTONS; i++) {
        pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    }

    // OLED init
    Wire.begin();
    delay(100);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("ERROR: OLED not found! Check wiring on A4/A5."));
    } else {
        Serial.println(F("OLED: OK"));
        oledOK = true;
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println(F("Hello World!"));
        display.println(F("Controller Test"));
        display.println(F("Serial: 115200"));
        display.display();
    }

    // Calibrate joystick centers
    delay(200);
    lxCenter = analogRead(L_JOY_X);
    lyCenter = analogRead(L_JOY_Y);
    rxCenter = analogRead(R_JOY_X);
    ryCenter = analogRead(R_JOY_Y);
    Serial.print(F("Joystick centers - LX:"));
    Serial.print(lxCenter);
    Serial.print(F(" LY:"));
    Serial.print(lyCenter);
    Serial.print(F(" RX:"));
    Serial.print(rxCenter);
    Serial.print(F(" RY:"));
    Serial.println(ryCenter);
    // NRF24 init
    if (!radio.begin()) {
        Serial.println(F("NRF24: INIT FAILED"));
    } else {
        radio.setPALevel(RF24_PA_LOW);
        radio.setDataRate(RF24_250KBPS);
        radio.setChannel(76);
        radio.setAutoAck(false);
        radio.openWritingPipe(rfAddress);
        radio.stopListening();
        Serial.println(F("NRF24: TX ready"));
    }

    Serial.println(F("--------------------------------"));
    Serial.println(F("Move joysticks and press buttons"));
    Serial.println(F("--------------------------------"));
}

void loop() {
    // --- TX packet ---
    unsigned long now = millis();
    if (now - lastTxTime >= TX_INTERVAL) {
        lastTxTime = now;
        txCounter++;
        bool ok = radio.write(&txCounter, sizeof(txCounter));
        if (ok) txOk++; else txFail++;
        Serial.print(F("TX ")); Serial.print(ok ? F("OK") : F("FAIL"));
        Serial.print(F(" cnt=")); Serial.print(txCounter);
        Serial.print(F(" OK=")); Serial.print(txOk);
        Serial.print(F(" FAIL=")); Serial.println(txFail);
    }

    // --- Read joystick raw values ---
    int lx = analogRead(L_JOY_X);
    int ly = analogRead(L_JOY_Y);
    int rx = analogRead(R_JOY_X);
    int ry = analogRead(R_JOY_Y);

    // Map to -100 to +100
    int lxMapped = map(lx, 0, 1023, -100, 100);
    int lyMapped = map(ly, 0, 1023, -100, 100);
    int rxMapped = map(rx, 0, 1023, -100, 100);
    int ryMapped = map(ry, 0, 1023, -100, 100);

    // --- Print joystick values to Serial ---
    Serial.print(F("LX:"));
    Serial.print(lxMapped);
    Serial.print(F("\tLY:"));
    Serial.print(lyMapped);
    Serial.print(F("\tRX:"));
    Serial.print(rxMapped);
    Serial.print(F("\tRY:"));
    Serial.print(ryMapped);
    Serial.print(F("\t"));

    // --- Check buttons (print on press only) ---
    bool anyPressed = false;
    for (int i = 0; i < NUM_BUTTONS; i++) {
        bool currentState = digitalRead(BUTTON_PINS[i]);
        if (currentState == LOW && lastButtonState[i] == HIGH) {
            Serial.print(F("BTN:"));
            Serial.print(BUTTON_NAMES[i]);
            Serial.print(F(" PRESSED  "));
            anyPressed = true;
        }
        if (currentState == HIGH && lastButtonState[i] == LOW) {
            Serial.print(F("BTN:"));
            Serial.print(BUTTON_NAMES[i]);
            Serial.print(F(" released  "));
        }
        lastButtonState[i] = currentState;
    }
    Serial.println();

    // --- Update OLED ---
    if (oledOK) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);

        // Row 1: TX status
        display.print(F("TX:"));
        display.print(txOk);
        display.print(F("/"));
        display.print(txFail);
        display.print(F(" cnt="));
        display.println(txCounter);

        // Row 2-3: Joysticks
        display.print(F("LX:")); display.print(lxMapped);
        display.print(F(" LY:")); display.println(lyMapped);
        display.print(F("RX:")); display.print(rxMapped);
        display.print(F(" RY:")); display.println(ryMapped);

        // Row 4-5: Buttons
        display.print(F("BTN:"));
        bool anyHeld = false;
        for (int i = 0; i < NUM_BUTTONS; i++) {
            if (digitalRead(BUTTON_PINS[i]) == LOW) {
                display.print(BUTTON_NAMES[i]);
                display.print(F(" "));
                anyHeld = true;
            }
        }
        if (!anyHeld) display.print(F("none"));

        display.display();
    }

    delay(50);
}
