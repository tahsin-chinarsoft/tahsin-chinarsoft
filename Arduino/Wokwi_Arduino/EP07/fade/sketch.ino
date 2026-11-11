// Arduino in Your Browser — Episode 7: a real fade on pin 9
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// LED on pin 9 (a ~ PWM pin). Now it fades smoothly from 0 to 255 and back.

const int LED = 9;

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
