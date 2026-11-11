// Episode 18 - OLED live graph, first try (the graph comes out upside down)
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);
const int POT = A0;
byte history[128];                          // one value per column
int head = 0;

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
}

void loop() {
  int value = analogRead(POT);              // 0 .. 1023
  history[head] = value / 16;               // 0 .. 63, one pixel per row
  head = (head + 1) % 128;

  display.clearDisplay();
  for (int x = 0; x < 128; x++) {
    int y = history[(head + x) % 128];      // oldest on the left
    display.drawPixel(x, y, SSD1306_WHITE);
  }
  display.display();
  delay(50);
}
