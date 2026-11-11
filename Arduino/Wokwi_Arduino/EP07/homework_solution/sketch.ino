// Arduino in Your Browser — Episode 7 HOMEWORK SOLUTION: red -> blue -> red with millis()
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// RGB LED (common cathode) on pins 9 (red) and 11 (blue). No delay():
// the position in a 4-second cycle comes from millis().

const int RED = 9, BLUE = 11;
const unsigned long HALF = 2000;          // 2 s each way
unsigned long lastPrint = 0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  unsigned long t = millis() % (2 * HALF);    // 0 ... 3999
  int x;                                      // 0 = red, 255 = blue
  if (t < HALF) x = t * 255 / HALF;           // red -> blue
  else          x = (2 * HALF - t) * 255 / HALF;  // blue -> red

  analogWrite(RED, 255 - x);
  analogWrite(BLUE, x);

  if (millis() - lastPrint >= 250) {
    lastPrint = millis();
    Serial.print("R ");  Serial.print(255 - x);
    Serial.print("   B "); Serial.println(x);
  }
}
