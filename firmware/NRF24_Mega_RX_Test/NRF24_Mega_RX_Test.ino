// ============================================================
// NRF24 Ping Test — MEGA RECEIVER
// Listens for counter packets from Uno. Prints received values to Serial.
// Wire NRF24 to: CE=D8, CSN=D9, MOSI=D51, MISO=D50, SCK=D52
// Requires: RF24 library by TMRh20 (Sketch → Include Library → Manage Libraries)
// ============================================================

#include <SPI.h>
#include <RF24.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool oledOK = false;

// --- NRF24 pins ---
#define CE_PIN  8
#define CSN_PIN 9
RF24 radio(CE_PIN, CSN_PIN);

// --- Radio address (must match transmitter) ---
const byte address[6] = "00001";

// --- Data packet ---
unsigned long receivedCounter = 0;
unsigned long rxCount = 0;
unsigned long lastRxTime = 0;

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }
    Serial.println(F("=== NRF24 RX Test (Mega) ==="));

    // OLED init
    Wire.begin();
    delay(100);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("OLED: NOT FOUND"));
    } else {
        Serial.println(F("OLED: OK"));
        oledOK = true;
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println(F("NRF24 RX TEST"));
        display.display();
    }

    if (!radio.begin()) {
        Serial.println(F("ERROR: NRF24 not found! Check wiring."));
        while (1) {} // Halt
    }

    radio.setPALevel(RF24_PA_LOW);
    radio.setDataRate(RF24_250KBPS);
    radio.setChannel(76);
    radio.setAutoAck(false);
    radio.openReadingPipe(0, address);
    radio.startListening();

    Serial.println(F("NRF24: Initialized OK"));
    Serial.println(F("Listening for packets from Uno..."));
}

void loop() {
    if (radio.available()) {
        radio.read(&receivedCounter, sizeof(receivedCounter));
        rxCount++;
        lastRxTime = millis();
        Serial.print(F("RX OK #")); Serial.print(rxCount);
        Serial.print(F(" counter=")); Serial.println(receivedCounter);
    }

    // Update OLED every 200ms
    static unsigned long lastOledUpdate = 0;
    unsigned long now = millis();
    if (oledOK && now - lastOledUpdate >= 200) {
        lastOledUpdate = now;
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println(F("NRF24 RX TEST"));
        display.print(F("RX# ")); display.println(rxCount);
        display.print(F("CNT ")); display.println(receivedCounter);
        if (rxCount > 0) {
            display.print(F("AGO ")); display.print((now - lastRxTime) / 1000); display.println(F("s"));
        } else {
            display.println(F("WAITING..."));
        }
        display.display();
    }
}
