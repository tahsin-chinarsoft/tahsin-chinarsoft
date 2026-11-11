// Episode 12 - servo control with a potentiometer
#include <Servo.h>

const int SERVO_PIN = 9;
const int POT = A0;
Servo myServo;

void setup() {
  Serial.begin(9600);
  myServo.attach(SERVO_PIN);   // starts the 50 Hz servo pulses
}

void loop() {
  int raw = analogRead(POT);                 // 0 ... 1023
  int angle = map(raw, 0, 1023, 0, 180);     // 0 ... 180 degrees
  myServo.write(angle);
  Serial.print("pot ");
  Serial.print(raw);
  Serial.print("  ->  ");
  Serial.print(angle);
  Serial.println(" degrees");
  delay(20);
}
