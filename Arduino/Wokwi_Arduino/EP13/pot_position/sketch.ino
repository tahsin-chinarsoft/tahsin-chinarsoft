// Episode 13 - the knob sets a position, the stepper counts its way there
const int DIR_PIN = 2;
const int STEP_PIN = 3;
const int POT = A0;
const unsigned long STEP_MS = 5;   // 200 steps per second

long position = 0;                 // steps from the start
unsigned long lastStep = 0;
unsigned long lastPrint = 0;

void setup() {
  Serial.begin(9600);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
}

void loop() {
  long target = map(analogRead(POT), 0, 1023, 0, 200);   // 0 ... one turn

  if (position != target && millis() - lastStep >= STEP_MS) {
    lastStep = millis();
    if (target > position) {
      digitalWrite(DIR_PIN, HIGH);
      position++;
    } else {
      digitalWrite(DIR_PIN, LOW);
      position--;
    }
    digitalWrite(STEP_PIN, HIGH);  // one pulse = one step
    delayMicroseconds(10);
    digitalWrite(STEP_PIN, LOW);
  }

  if (millis() - lastPrint >= 200) {
    lastPrint = millis();
    Serial.print("target ");
    Serial.print(target);
    Serial.print("   position ");
    Serial.print(position);
    Serial.print("   angle ");
    Serial.println(position * 1.8);
  }
}
