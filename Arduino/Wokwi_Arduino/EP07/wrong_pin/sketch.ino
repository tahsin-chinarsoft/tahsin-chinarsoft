// Arduino in Your Browser — Episode 7: the fade that does not fade
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// LED on pin 12 (no ~ sign). Watch: it never fades, it just switches on and off.

const int LED = 12;

void setup() {
  Serial.begin(9600);
  pinMode(LED, OUTPUT);
}

void loop() {
  for (int b = 0; b <= 255; b += 5) {     // fade up
    analogWrite(LED, b);
    Serial.println(b);
    delay(40);
  }
  for (int b = 255; b >= 0; b -= 5) {     // fade down
    analogWrite(LED, b);
    Serial.println(b);
    delay(40);
  }
}
