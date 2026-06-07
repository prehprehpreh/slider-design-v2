// ============================================================
// NRF24 Ping Test — UNO / NANO TRANSMITTER
// Sends a counter every 500ms. Wire NRF24 to pins per wiring diagram.
// Requires: RF24 library by TMRh20 (Sketch → Include Library → Manage Libraries)
// ============================================================

#include <SPI.h>
#include <RF24.h>

// --- NRF24 pins (match wiring diagram) ---
#define CE_PIN  8
#define CSN_PIN 9
RF24 radio(CE_PIN, CSN_PIN);

// --- Radio address (must match receiver) ---
const byte address[6] = "00001";

// --- Data packet ---
unsigned long counter = 0;

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }
    Serial.println(F("=== NRF24 TX Test (Uno) ==="));

    if (!radio.begin()) {
        Serial.println(F("ERROR: NRF24 not found! Check wiring."));
        while (1) {} // Halt
    }

    radio.setPALevel(RF24_PA_LOW);   // Low power for close-range testing
    radio.setDataRate(RF24_250KBPS); // Slower = longer range, more reliable
    radio.setChannel(76);            // Channel 76 (default, away from WiFi)
    radio.setAutoAck(false);         // Disable ACK for simple one-way test
    radio.openWritingPipe(address);
    radio.stopListening();

    Serial.println(F("NRF24: Initialized OK"));
    Serial.println(F("Sending to Mega every 500ms..."));
}

void loop() {
    counter++;

    bool ok = radio.write(&counter, sizeof(counter));

    if (ok) {
        Serial.print(F("TX OK  counter="));
        Serial.println(counter);
    } else {
        Serial.print(F("TX FAIL counter="));
        Serial.println(counter);
    }

    delay(500);
}
