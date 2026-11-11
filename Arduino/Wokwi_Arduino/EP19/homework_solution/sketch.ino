// Episode 19 homework solution - level meter: the pot lights 0..16 pixels, green, yellow, red
#include <Adafruit_NeoPixel.h>

const int PIN = 6, COUNT = 16, POT = A0;
Adafruit_NeoPixel ring(COUNT, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  ring.begin();
  ring.setBrightness(80);
}

void loop() {
  int lit = map(analogRead(POT), 0, 1023, 0, COUNT);   // how many pixels
  for (int i = 0; i < COUNT; i++) {
    uint32_t c = 0;                                    // off
    if (i < lit) {
      if (i < 10) c = ring.Color(0, 255, 0);           // green
      else if (i < 13) c = ring.Color(255, 160, 0);    // yellow
      else c = ring.Color(255, 0, 0);                  // red
    }
    ring.setPixelColor(i, c);
  }
  ring.show();
  delay(20);
}
