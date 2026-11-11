// Arduino in Your Browser — Episode 7: mix any color with an RGB LED
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// Three knobs = red, green, blue. analogWrite sets each brightness 0 ... 255.
// RGB LED (common cathode) on the ~ PWM pins 9, 10, 11.

const int RED = 9, GREEN = 10, BLUE = 11;
unsigned long lastPrint = 0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  int r = map(analogRead(A0), 0, 1023, 0, 255);   // 10-bit in, 8-bit out
  int g = map(analogRead(A1), 0, 1023, 0, 255);
  int b = map(analogRead(A2), 0, 1023, 0, 255);

  analogWrite(RED, r);
  analogWrite(GREEN, g);
  analogWrite(BLUE, b);

  if (millis() - lastPrint >= 300) {
    lastPrint = millis();
    Serial.print("R ");  Serial.print(r);
    Serial.print("   G "); Serial.print(g);
    Serial.print("   B "); Serial.println(b);
  }
}
