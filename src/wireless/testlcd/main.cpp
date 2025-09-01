#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "images.h"

// OLED display size
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// OLED reset pin (set to -1 if not used)
#define OLED_RESET    -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Example 16x16 pixel bitmap image (a simple smiley face)

void setup() {
    // Initialize display
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
         display.println("startup");
        for(;;); // Don't proceed, loop forever
    }

    display.clearDisplay();

    // Draw bitmap image
    display.drawBitmap(0, 0, my_image, 128, 64, SSD1306_WHITE);

    display.display();
}

void loop() {
    // Nothing to do here
}