// Episode 19 - NeoPixel ring: a spinning rainbow, brightness from a potentiometer
#include <Adafruit_NeoPixel.h>

const int PIN = 6, COUNT = 16;
const int POT = A0;
Adafruit_NeoPixel ring(COUNT, PIN, NEO_GRB + NEO_KHZ800);   // most rings are GRB

uint16_t start = 0;                         // where the rainbow starts (0..65535)
unsigned long lastFrame = 0;

void setup() {
  ring.begin();
  ring.show();                              // all pixels off
}

void loop() {
  if (millis() - lastFrame < 20) return;    // 50 frames per second
  lastFrame = millis();

  int level = map(analogRead(POT), 0, 1023, 0, 255);
  ring.setBrightness(level);                // scales every color down

  for (int i = 0; i < COUNT; i++) {
    uint16_t hue = start + i * 65536L / COUNT;   // spread the color wheel around the ring
    ring.setPixelColor(i, ring.gamma32(ring.ColorHSV(hue)));
  }
  ring.show();                              // nothing changes until show()
  start += 256;                             // turn the wheel a little
}
