// I2C Scanner - finds address of all connected I2C devices
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
    Serial.begin(115200);
    Wire.begin();
    Serial.println("Scanning I2C bus...");

    int found = 0;
    for (byte addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        byte error = Wire.endTransmission();
        if (error == 0) {
            Serial.print("Device found at 0x");
            if (addr < 16) Serial.print("0");
            Serial.println(addr, HEX);
            found++;
        }
    }
    if (found == 0) {
        Serial.println("No I2C devices found. Check SDA=A4 and SCL=A5 wiring.");
    } else {
        Serial.println("Scan complete.");
        if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
            display.clearDisplay();
            display.setTextSize(2);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0, 20);
            display.println("Hello");
            display.println("World!");
            display.display();
            Serial.println("OLED: Hello World displayed.");
        } else {
            Serial.println("OLED init failed.");
        }
    }
}

void loop() {}
