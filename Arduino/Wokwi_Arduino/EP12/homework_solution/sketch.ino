// Episode 12 homework solution - a slow servo
// The servo walks to the knob's angle: 1 degree every 15 ms, no delay.
#include <Servo.h>

const int SERVO_PIN = 9;
const int POT = A0;
const unsigned long STEP_MS = 15;

Servo myServo;
int current = 90;                 // where the servo is now
unsigned long lastStep = 0;
unsigned long lastPrint = 0;

void setup() {
  Serial.begin(9600);
  myServo.attach(SERVO_PIN);
  myServo.write(current);
}

void loop() {
  int target = map(analogRead(POT), 0, 1023, 0, 180);

  if (millis() - lastStep >= STEP_MS) {
    lastStep = millis();
    if (current < target) current++;        // one degree closer
    else if (current > target) current--;
    myServo.write(current);
  }

  if (millis() - lastPrint >= 200) {
    lastPrint = millis();
    Serial.print("target ");
    Serial.print(target);
    Serial.print("   servo ");
    Serial.println(current);
  }
}
