// Episode 19 - NeoPixel ring, first try: I asked for red...
#include <Adafruit_NeoPixel.h>

const int PIN = 6, COUNT = 16;
Adafruit_NeoPixel ring(COUNT, PIN, NEO_RGB + NEO_KHZ800);   // wrong color order for this ring

void setup() {
  ring.begin();
  for (int i = 0; i < COUNT; i++) ring.setPixelColor(i, ring.Color(255, 0, 0));   // red, please
  ring.show();
}

void loop() {}
