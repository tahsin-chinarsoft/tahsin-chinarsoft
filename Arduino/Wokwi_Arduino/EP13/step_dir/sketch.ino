// Episode 13 - stepper basics: one pulse on STEP = one step
const int DIR_PIN = 2;
const int STEP_PIN = 3;
const int STEPS_PER_REV = 200;     // 1.8 degrees per step

void setup() {
  Serial.begin(9600);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
}

void stepMotor(int steps) {
  for (int i = 0; i < steps; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(STEP_PIN, LOW);
    delay(5);                      // 5 ms per step = speed
  }
}

void loop() {
  digitalWrite(DIR_PIN, HIGH);     // one direction
  for (int q = 1; q <= 4; q++) {
    stepMotor(50);                 // 50 steps = a quarter turn
    Serial.print("quarter turn ");
    Serial.println(q);
    delay(1000);
  }
  digitalWrite(DIR_PIN, LOW);      // the other direction
  stepMotor(STEPS_PER_REV);        // one full turn back
  Serial.println("full turn back");
  delay(1000);
}
