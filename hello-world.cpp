
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

  // Clear screen
  tft.fillScreen(GC9A01A_BLACK);

  // Display Hello World
  tft.setTextColor(GC9A01A_WHITE);
  tft.setTextSize(2);
  tft.setCursor(55, 110);
  tft.println("Hello World");
}

void loop() {
  // Nothing needed here
}
