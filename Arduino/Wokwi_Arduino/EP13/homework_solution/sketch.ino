// Episode 13 homework solution - a quarter turn per button press
// Bonus: every press goes the other way. millis(), no delay().
const int DIR_PIN = 2;
const int STEP_PIN = 3;
const int BUTTON = 4;                // to GND, INPUT_PULLUP
const int QUARTER = 50;              // 50 steps = 90 degrees

int stepsLeft = 0;                   // steps still to do
int dir = 1;                         // +1 or -1
long position = 0;
bool lastButton = HIGH;
unsigned long lastChange = 0;
unsigned long lastStep = 0;

void setup() {
  Serial.begin(9600);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
}

void loop() {
  // 1) button: react to a new press (debounced edge)
  bool b = digitalRead(BUTTON);
  if (b != lastButton && millis() - lastChange > 50) {
    lastChange = millis();
    lastButton = b;
    if (b == LOW && stepsLeft == 0) {
      stepsLeft = QUARTER;
      digitalWrite(DIR_PIN, dir > 0 ? HIGH : LOW);
      Serial.println(dir > 0 ? "press: quarter turn forward" : "press: quarter turn back");
    }
  }

  // 2) motor: one step every 5 ms while steps are left
  if (stepsLeft > 0 && millis() - lastStep >= 5) {
    lastStep = millis();
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(STEP_PIN, LOW);
    position += dir;
    stepsLeft--;
    if (stepsLeft == 0) {
      Serial.print("position ");
      Serial.print(position);
      Serial.print(" steps = ");
      Serial.print(position * 1.8, 0);
      Serial.println(" degrees");
      dir = -dir;                    // bonus: next press goes the other way
    }
  }
}
