// Episode 18 homework solution - min and max of the visible graph, and an alarm line at 800
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int W = 128, H = 64, TOP = 16;
const int ALARM = 800;                      // above this, the LED on pin 13 lights
Adafruit_SSD1306 display(W, H, &Wire, -1);
const int POT = A0, LED = 13;

int history[W];                             // raw values this time (0..1023)
int head = 0;
unsigned long lastSample = 0;

int toY(int v) { return map(v, 0, 1023, H - 1, TOP); }

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(SSD1306_WHITE);
  pinMode(LED, OUTPUT);
  for (int x = 0; x < W; x++) history[x] = 0;
}

void loop() {
  if (millis() - lastSample < 50) return;
  lastSample = millis();

  int value = analogRead(POT);
  history[head] = value;
  head = (head + 1) % W;
  digitalWrite(LED, value > ALARM);

  int lo = 1023, hi = 0;                    // min and max of what is on screen
  for (int x = 0; x < W; x++) {
    lo = min(lo, history[x]);
    hi = max(hi, history[x]);
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("min ");
  display.print(lo);
  display.print("  max ");
  display.print(hi);
  for (int x = 0; x < W; x += 4) display.drawPixel(x, toY(ALARM), SSD1306_WHITE);  // dashed line
  for (int x = 1; x < W; x++) {
    display.drawLine(x - 1, toY(history[(head + x - 1) % W]), x, toY(history[(head + x) % W]), SSD1306_WHITE);
  }
  display.display();
}
