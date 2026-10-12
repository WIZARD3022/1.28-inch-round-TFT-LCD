
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
}

void loop() {
  // RED
  tft.fillScreen(GC9A01A_RED);
  showColor("RED");
  delay(2000);

  // GREEN
  tft.fillScreen(GC9A01A_GREEN);
  showColor("GREEN");
  delay(2000);

  // BLUE
  tft.fillScreen(GC9A01A_BLUE);
  showColor("BLUE");
  delay(2000);

  // WHITE
  tft.fillScreen(GC9A01A_WHITE);
  showColor("WHITE");
  delay(2000);

  // BLACK
  tft.fillScreen(GC9A01A_BLACK);
  showColor("BLACK");
  delay(2000);

  // YELLOW
  tft.fillScreen(0xFFE0);
  showColor("YELLOW");
  delay(2000);

  // CYAN
  tft.fillScreen(0x07FF);
  showColor("CYAN");
  delay(2000);

  // MAGENTA
  tft.fillScreen(0xF81F);
  showColor("MAGENTA");
  delay(2000);
}

void showColor(const char *name) {
  tft.setTextColor(
    (strcmp(name, "BLACK") == 0 ||
     strcmp(name, "BLUE") == 0 ||
     strcmp(name, "RED") == 0)
       ? GC9A01A_WHITE : GC9A01A_BLACK
  );

  tft.setTextSize(2);
  tft.setCursor(65, 115);
  tft.print(name);
}
