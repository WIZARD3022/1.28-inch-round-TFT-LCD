
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>

#define TFT_CS   10
#define TFT_DC    8
#define TFT_RST   9

Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  tft.begin();
  tft.setRotation(0);

  // Black background
  tft.fillScreen(GC9A01A_BLACK);

  // Yellow face
  tft.fillCircle(120, 120, 85, 0xFFE0);

  // Eyes
  tft.fillCircle(90, 95, 9, GC9A01A_BLACK);
  tft.fillCircle(150, 95, 9, GC9A01A_BLACK);

  // Smiling mouth using an arc
  for (int angle = 20; angle <= 160; angle++) {
    float rad = angle * 3.14159 / 180.0;

    int x = 120 + cos(rad) * 45;
    int y = 125 + sin(rad) * 45;

    tft.fillCircle(x, y, 3, GC9A01A_BLACK);
  }
}

void loop() {
  // Face remains on screen
}
