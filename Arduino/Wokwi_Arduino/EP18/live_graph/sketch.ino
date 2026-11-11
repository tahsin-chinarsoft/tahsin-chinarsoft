// Episode 18 - OLED live graph of a potentiometer (SSD1306 128x64, I2C)
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int W = 128, H = 64;
const int TOP = 16;                         // rows 0..15: text, rows 16..63: graph
Adafruit_SSD1306 display(W, H, &Wire, -1);
const int POT = A0;

byte history[W];                            // graph height of each column
int head = 0;
unsigned long lastSample = 0;

void setup() {
  Serial.begin(9600);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 not found");
    while (true);
  }
  display.setTextColor(SSD1306_WHITE);
  for (int x = 0; x < W; x++) history[x] = H - 1;   // start flat at the bottom
}

void loop() {
  if (millis() - lastSample < 50) return;   // 20 samples per second
  lastSample = millis();

  int value = analogRead(POT);
  // screen y grows DOWN: 1023 must be the top row of the graph, 0 the bottom row
  history[head] = map(value, 0, 1023, H - 1, TOP);
  head = (head + 1) % W;

  display.clearDisplay();                   // 1) draw everything in memory...
  display.setCursor(0, 0);
  display.print("A0: ");
  display.print(value);
  display.drawFastHLine(0, TOP - 2, W, SSD1306_WHITE);
  for (int x = 1; x < W; x++) {             // oldest on the left, newest on the right
    int y0 = history[(head + x - 1) % W];
    int y1 = history[(head + x) % W];
    display.drawLine(x - 1, y0, x, y1, SSD1306_WHITE);
  }
  display.display();                        // 2) ...then send it all at once
}
