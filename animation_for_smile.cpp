
#include <SPI.h>
#include <math.h>
#include <stdlib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>

// ---------- GC9A01A 1.28-INCH TFT ----------
#define TFT_CS   10
#define TFT_DC    8
#define TFT_RST   9

Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);

// ---------- COLOURS ----------
#define BLACK     0x0000
#define WHITE     0xFFFF
#define NEON      0x07FF
#define BLUE      0x03FF
#define DARKBLUE  0x0010
#define RED       0xF800
#define ORANGE    0xFD20
#define GREEN     0x07E0
#define YELLOW    0xFFE0
#define PURPLE    0xF81F

// ---------- SCREEN LAYOUT ----------
const int SCREEN_W = 240;
const int SCREEN_H = 240;

const int FACE_X = 78;
const int FACE_Y = 126;
const int FACE_R = 51;

const int ICON_X = 180;
const int ICON_Y = 145;

int statusCode = 0;

// ---------- ANIMATION ----------
unsigned long lastFrame = 0;
const unsigned long FRAME_MS = 25; // 40 updates/sec

float orbitAngle = 0.0;
int oldDotX = -1;
int oldDotY = -1;

int oldProgressX = -1;
const int PROGRESS_Y = 225;

// Serial input
char inputBuffer[8];
byte inputLength = 0;

// ---------- STATUS NAMES ----------
const char* statusNames[17] = {
  "DEFAULT SMILE",
  "HIGH CPU USAGE",
  "HIGH RAM USAGE",
  "DISK SPACE FULL",
  "NETWORK FAILURE",
  "DATABASE FAILURE",
  "SECURITY BLOCK",
  "SYSTEM LOG ERROR",
  "SSL CERTIFICATE",
  "DNS FAILURE",
  "SERVER CRASH",
  "REQUEST TIMEOUT",
  "BACKUP FAILURE",
  "STARTING PRINT",
  "PRINTER NOT FOUND",
  "PRINT COMPLETE",
  "MONEY RECEIVED"
};

// ---------- DRAWING HELPERS ----------

void neonLine(int x0, int y0, int x1, int y1) {
  tft.drawLine(x0, y0, x1, y1, DARKBLUE);
  tft.drawLine(x0, y0, x1, y1, BLUE);
  tft.drawLine(x0, y0, x1, y1, NEON);
}

void thickLine(int x0, int y0, int x1, int y1,
               uint16_t colour, int thickness) {
  int dx = x1 - x0;
  int dy = y1 - y0;
  int steps = max(abs(dx), abs(dy));

  if (steps == 0) {
    tft.fillCircle(x0, y0, thickness, colour);
    return;
  }

  for (int i = 0; i <= steps; i++) {
    int x = x0 + (long)dx * i / steps;
    int y = y0 + (long)dy * i / steps;
    tft.fillCircle(x, y, thickness, colour);
  }
}

void neonCircle(int x, int y, int r) {
  tft.drawCircle(x, y, r + 2, DARKBLUE);
  tft.drawCircle(x, y, r + 1, BLUE);
  tft.drawCircle(x, y, r, NEON);
}

void neonRect(int x, int y, int w, int h) {
  tft.drawRect(x - 2, y - 2, w + 4, h + 4, DARKBLUE);
  tft.drawRect(x - 1, y - 1, w + 2, h + 2, BLUE);
  tft.drawRect(x, y, w, h, NEON);
}

void arcLine(int x, int y, int rx, int ry,
             float start, float finish,
             uint16_t colour, int thickness = 1) {
  int px = x + (int)(cos(start) * rx);
  int py = y + (int)(sin(start) * ry);

  for (int i = 1; i <= 30; i++) {
    float a = start + (finish - start) * i / 30.0;
    int nx = x + (int)(cos(a) * rx);
    int ny = y + (int)(sin(a) * ry);

    thickLine(px, py, nx, ny, colour, thickness);
    px = nx;
    py = ny;
  }
}

void smile(int x, int y, int w, int h) {
  arcLine(x, y, w, h, 0.12, PI - 0.12, DARKBLUE, 3);
  arcLine(x, y, w, h, 0.12, PI - 0.12, BLUE, 2);
  arcLine(x, y, w, h, 0.12, PI - 0.12, NEON, 1);
}

void frown(int x, int y) {
  arcLine(x, y, 16, 10,
          PI + 0.15, 2 * PI - 0.15, BLUE, 2);
  arcLine(x, y, 16, 10,
          PI + 0.15, 2 * PI - 0.15, NEON, 1);
}

void drawX(int x, int y, int size) {
  neonLine(x - size, y - size, x + size, y + size);
  neonLine(x + size, y - size, x - size, y + size);
}

void sparkle(int x, int y, int r) {
  neonLine(x - r, y, x + r, y);
  neonLine(x, y - r, x, y + r);
}

// ---------- FACE ----------

void drawFace() {
  // Static neon outline: no frame-by-frame redraw
  neonCircle(FACE_X, FACE_Y, FACE_R);

  bool happy =
    statusCode == 0  ||
    statusCode == 6  ||
    statusCode == 13 ||
    statusCode == 15 ||
    statusCode == 16;

  if (statusCode == 10) {
    // Server crash: X-shaped eyes remain visible
    drawX(FACE_X - 17, FACE_Y - 12, 4);
    drawX(FACE_X + 17, FACE_Y - 12, 4);
  }
  else if (happy) {
    // Happy curved eyes
    arcLine(FACE_X - 17, FACE_Y - 12, 6, 5,
            PI, 2 * PI, NEON, 2);
    arcLine(FACE_X + 17, FACE_Y - 12, 6, 5,
            PI, 2 * PI, NEON, 2);
  }
  else {
    // Worried eyes
    tft.fillCircle(FACE_X - 17, FACE_Y - 12, 5, BLUE);
    tft.fillCircle(FACE_X - 17, FACE_Y - 12, 3, NEON);
    tft.fillCircle(FACE_X + 17, FACE_Y - 12, 5, BLUE);
    tft.fillCircle(FACE_X + 17, FACE_Y - 12, 3, NEON);

    // Worried eyebrows
    neonLine(FACE_X - 26, FACE_Y - 25,
             FACE_X - 13, FACE_Y - 28);
    neonLine(FACE_X + 13, FACE_Y - 28,
             FACE_X + 26, FACE_Y - 25);
  }

  if (happy) {
    smile(FACE_X, FACE_Y + 8, 17, 13);
  } else {
    frown(FACE_X, FACE_Y + 28);
  }

  // Permanent sweat drops for CPU and RAM events
  if (statusCode == 1 || statusCode == 2) {
    neonLine(FACE_X - 33, FACE_Y - 25,
             FACE_X - 37, FACE_Y - 16);
    neonLine(FACE_X - 37, FACE_Y - 16,
             FACE_X - 33, FACE_Y - 12);
  }

  // Confused expressions
  if (statusCode == 9 || statusCode == 14) {
    tft.setTextColor(NEON);
    tft.setTextSize(2);
    tft.setCursor(FACE_X - 43, FACE_Y - 48);
    tft.print("?");
  }
}

// ---------- EVENT ICONS ----------

void drawCPU() {
  int x = ICON_X, y = ICON_Y;
  neonRect(x - 17, y - 17, 34, 34);

  for (int i = 0; i < 4; i++) {
    int a = x - 12 + i * 8;
    neonLine(a, y - 23, a, y - 17);
    neonLine(a, y + 17, a, y + 23);
    neonLine(x - 23, y - 12 + i * 8,
             x - 17, y - 12 + i * 8);
    neonLine(x + 17, y - 12 + i * 8,
             x + 23, y - 12 + i * 8);
  }

  tft.setTextSize(1);
  tft.setTextColor(NEON);
  tft.setCursor(x - 9, y - 3);
  tft.print("CPU");

  // Steady heat waves, no blinking
  for (int i = 0; i < 3; i++) {
    int hx = x - 18 + i * 12;
    neonLine(hx, y - 29, hx - 4, y - 36);
    neonLine(hx - 4, y - 36, hx, y - 43);
  }
}

void drawRAM() {
  int x = ICON_X, y = ICON_Y;
  neonRect(x - 23, y - 10, 46, 20);

  for (int i = 0; i < 6; i++) {
    int xx = x - 17 + i * 7;
    neonLine(xx, y - 5, xx, y + 4);
    neonLine(xx, y + 10, xx, y + 15);
  }

  // Fixed upward arrows
  for (int i = 0; i < 2; i++) {
    int ax = x - 10 + i * 20;
    neonLine(ax, y - 18, ax, y - 37);
    neonLine(ax, y - 37, ax - 5, y - 30);
    neonLine(ax, y - 37, ax + 5, y - 30);
  }
}


void drawDisk(bool warning) {
  int x = ICON_X, y = ICON_Y;

  // Disk cylinder
  tft.drawRoundRect(x - 21, y - 25, 42, 51, 8, BLUE);
  tft.drawRoundRect(x - 20, y - 24, 40, 49, 8, NEON);

  arcLine(x, y - 17, 20, 6, 0, PI, BLUE);
  arcLine(x, y + 1, 20, 6, 0, PI, BLUE);
  arcLine(x, y + 18, 20, 6, 0, PI, NEON);

  if (warning) {
    tft.fillCircle(x + 23, y + 15, 11, ORANGE);
    tft.fillCircle(x + 23, y + 15, 8, RED);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.setCursor(x + 20, y + 7);
    tft.print("!");
  } else {
    drawX(x + 22, y + 17, 6);
  }
}


void drawNetwork() {
  int x = ICON_X, y = ICON_Y;

  arcLine(x, y - 6, 28, 22, PI + 0.3, 2 * PI - 0.3, NEON, 2);
  arcLine(x, y - 3, 18, 14, PI + 0.3, 2 * PI - 0.3, BLUE, 2);
  tft.fillCircle(x, y + 8, 3, NEON);

  // Broken connection
  tft.drawRoundRect(x - 28, y + 16, 21, 10, 4, NEON);
  tft.drawRoundRect(x - 4, y + 16, 21, 10, 4, BLUE);
  neonLine(x - 8, y + 21, x - 4, y + 21);
  drawX(x + 25, y + 27, 6);
}

void drawShield() {
  int x = ICON_X, y = ICON_Y;

  neonLine(x, y - 30, x + 23, y - 17);
  neonLine(x + 23, y - 17, x + 19, y + 12);
  neonLine(x + 19, y + 12, x, y + 29);
  neonLine(x, y + 29, x - 19, y + 12);
  neonLine(x - 19, y + 12, x - 23, y - 17);
  neonLine(x - 23, y - 17, x, y - 30);

  drawX(x, y, 9);
}

void drawDocument() {
  int x = ICON_X - 18, y = ICON_Y - 30;

  neonRect(x, y, 37, 54);

  for (int i = 0; i < 4; i++) {
    neonLine(x + 6, y + 10 + i * 8,
             x + 27, y + 10 + i * 8);
  }

  int wx = ICON_X + 10, wy = ICON_Y + 20;
  neonLine(wx, wy - 12, wx - 12, wy + 9);
  neonLine(wx - 12, wy + 9, wx + 12, wy + 9);
  neonLine(wx + 12, wy + 9, wx, wy - 12);
  tft.setTextColor(ORANGE);
  tft.setTextSize(2);
  tft.setCursor(wx - 3, wy - 5);
  tft.print("!");
}

void drawCertificate() {
  int x = ICON_X - 20, y = ICON_Y - 31;
  neonRect(x, y, 42, 47);

  for (int i = 0; i < 3; i++) {
    neonLine(x + 5, y + 10 + i * 8,
             x + 30, y + 10 + i * 8);
  }

  neonCircle(x + 25, y + 40, 9);
  neonLine(x + 20, y + 47, x + 18, y + 57);
  neonLine(x + 30, y + 47, x + 33, y + 57);

  // Stable clock hands
  neonLine(x + 25, y + 40, x + 25, y + 34);
  neonLine(x + 25, y + 40, x + 30, y + 43);
}

void drawDNS() {
  int x = ICON_X, y = ICON_Y;

  neonCircle(x, y, 24);
  tft.drawCircle(x, y, 13, BLUE);
  tft.drawFastHLine(x - 22, y, 44, NEON);

  tft.setTextSize(1);
  tft.setTextColor(NEON);
  tft.setCursor(x - 10, y - 3);
  tft.print("DNS");

  drawX(x + 20, y + 25, 5);
}

void drawServer() {
  int x = ICON_X, y = ICON_Y;

  for (int i = 0; i < 3; i++) {
    int yy = y - 25 + i * 18;
    neonRect(x - 22, yy, 44, 14);
    tft.fillCircle(x - 13, yy + 7, 2, NEON);
    tft.fillCircle(x - 5, yy + 7, 2, BLUE);
  }

  drawX(x + 27, y + 19, 6);
}

void drawClock() {
  int x = ICON_X, y = ICON_Y;

  neonCircle(x, y, 23);

  for (int i = 0; i < 8; i++) {
    float a = i * PI / 4.0;
    int dx = (int)(cos(a) * 17);
    int dy = (int)(sin(a) * 17);
    tft.fillCircle(x + dx, y + dy, 1, NEON);
  }

  neonLine(x, y, x, y - 13);
  neonLine(x, y, x + 10, y + 5);
}

void drawCloud() {
  int x = ICON_X, y = ICON_Y - 10;

  tft.drawCircle(x - 14, y, 12, BLUE);
  tft.drawCircle(x, y - 9, 16, NEON);
  tft.drawCircle(x + 15, y, 11, BLUE);
  neonLine(x - 25, y + 10, x + 25, y + 10);

  tft.drawRoundRect(x - 19, y + 20, 38, 29, 6, NEON);
  arcLine(x, y + 26, 18, 5, 0, PI, BLUE);
  arcLine(x, y + 40, 18, 5, 0, PI, BLUE);

  drawX(x + 21, y + 32, 5);
}

void drawPrinter(bool complete, bool missing) {
  int x = ICON_X, y = ICON_Y;

  neonRect(x - 23, y - 7, 46, 29);
  neonRect(x - 15, y - 29, 30, 21);
  tft.fillCircle(x - 10, y - 20, 2, NEON);
  tft.fillCircle(x + 10, y - 20, 2, NEON);

  neonRect(x - 14, y + 13, 28, 21);
  neonLine(x - 7, y + 20, x + 7, y + 20);
  neonLine(x - 7, y + 26, x + 7, y + 26);

  if (complete) {
    tft.drawCircle(x + 22, y + 19, 12, GREEN);
    tft.drawCircle(x + 22, y + 19, 10, GREEN);
    neonLine(x + 15, y + 19, x + 20, y + 24);
    neonLine(x + 20, y + 24, x + 29, y + 13);
    sparkle(x - 28, y - 28, 4);
    sparkle(x + 28, y - 29, 3);
  }

  if (missing) {
    drawX(x + 23, y + 18, 6);
    tft.setTextSize(2);
    tft.setTextColor(NEON);
    tft.setCursor(x - 34, y - 43);
    tft.print("?");
  }
}

void drawMoney() {
  int x = ICON_X, y = ICON_Y;

  neonCircle(x, y, 22);
  tft.setTextSize(2);
  tft.setTextColor(NEON);
  tft.setCursor(x - 10, y - 8);
  tft.print("Rs");

  for (int i = 0; i < 6; i++) {
    float a = i * PI / 3.0;
    int sx = x + (int)(cos(a) * 34);
    int sy = y + (int)(sin(a) * 34);
    neonLine(sx, sy,
             sx + (int)(cos(a) * 5),
             sy + (int)(sin(a) * 5));
  }

  sparkle(x - 29, y - 28, 3);
  sparkle(x + 30, y + 28, 3);
}

void drawStatusIcon() {
  switch (statusCode) {
    case 0:
      neonCircle(ICON_X, ICON_Y, 17);
      sparkle(ICON_X + 27, ICON_Y - 28, 4);
      break;

    case 1:  drawCPU(); break;
    case 2:  drawRAM(); break;
    case 3:  drawDisk(true); break;
    case 4:  drawNetwork(); break;
    case 5:  drawDisk(false); break;
    case 6:  drawShield(); break;
    case 7:  drawDocument(); break;
    case 8:  drawCertificate(); break;
    case 9:  drawDNS(); break;
    case 10: drawServer(); break;
    case 11: drawClock(); break;
    case 12: drawCloud(); break;
    case 13: drawPrinter(false, false); break;
    case 14: drawPrinter(false, true); break;
    case 15: drawPrinter(true, false); break;
    case 16: drawMoney(); break;
  }
}

// ---------- RENDER ONLY ON STATUS CHANGE ----------

void renderStatus() {
  tft.fillScreen(BLACK);

  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.setCursor(8, 9);
  tft.print("#");
  tft.print(statusCode);
  tft.print(" ");
  tft.print(statusNames[statusCode]);

  drawFace();
  drawStatusIcon();

  // Reset animation markers after the background is drawn
  oldDotX = -1;
  oldDotY = -1;
  oldProgressX = -1;
}

// ---------- SMOOTH, NON-BLINKING ANIMATION ----------

void animate() {
  // Moving dot stays outside the face outline.
  // Erase only the old dot, not the whole screen.
  if (oldDotX >= 0) {
    tft.fillCircle(oldDotX, oldDotY, 2, BLACK);
  }

  orbitAngle += 0.055;
  if (orbitAngle >= 2 * PI) orbitAngle -= 2 * PI;

  int dotX = FACE_X + (int)(cos(orbitAngle) * 61);
  int dotY = FACE_Y + (int)(sin(orbitAngle) * 61);

  tft.fillCircle(dotX, dotY, 3, BLUE);
  tft.fillCircle(dotX, dotY, 1, NEON);

  oldDotX = dotX;
  oldDotY = dotY;

  // Progress indicator moves continuously along bottom.
  // Its previous position is erased locally.
  if (oldProgressX >= 0) {
    tft.fillCircle(oldProgressX, PROGRESS_Y, 2, BLACK);
  }

  int progressX = 10 + ((millis() / 25) % 220);
  tft.fillCircle(progressX, PROGRESS_Y, 2, NEON);
  oldProgressX = progressX;
}

// ---------- SERIAL INPUT ----------

void handleSerial() {
  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\r' || c == '\n') {
      if (inputLength == 0) continue;

      inputBuffer[inputLength] = '\0';
      int value = atoi(inputBuffer);
      inputLength = 0;

      if (value >= 0 && value <= 16) {
        statusCode = value;
        renderStatus();

        Serial.print(F("Selected "));
        Serial.print(statusCode);
        Serial.print(F(": "));
        Serial.println(statusNames[statusCode]);
      } else {
        Serial.println(F("Enter a number from 0 to 16."));
      }
    }
    else if (c >= '0' && c <= '9') {
      if (inputLength < sizeof(inputBuffer) - 1) {
        inputBuffer[inputLength++] = c;
      } else {
        inputLength = 0;
        Serial.println(F("Input too long. Try again."));
      }
    }
    else if (c != ' ' && c != '\t') {
      inputLength = 0;
      Serial.println(F("Numbers only, please."));
    }
  }
}

// ---------- SETUP ----------

void setup() {
  Serial.begin(115200);

  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(BLACK);

  Serial.println(F("\nNEON STATUS MASCOT"));
  Serial.println(F("Enter a code from 0 to 16, then press Enter."));
  Serial.println(F("0 = Default smile"));

  for (int i = 1; i <= 16; i++) {
    Serial.print(i);
    Serial.print(F(" = "));
    Serial.println(statusNames[i]);
  }

  renderStatus();
}

// ---------- MAIN LOOP ----------

void loop() {
  handleSerial();

  unsigned long now = millis();

  if (now - lastFrame >= FRAME_MS) {
    lastFrame = now;
    animate();
  }
}
