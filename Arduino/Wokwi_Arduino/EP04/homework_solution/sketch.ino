// Arduino in Your Browser — Episode 4 HOMEWORK SOLUTION: long press
// Short press: +1. Hold for 1 second: counter resets to 0. No delay().
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// INPUT_PULLUP + debounce with millis():
// a press only counts once the button has been steady for 50 ms.
// One press = one count, every time.

const int BUTTON = 2;      // button to GND, no external resistor
const int LED = 12;
const unsigned long DEBOUNCE_MS = 50;

int lastReading = HIGH;          // last raw reading (may be bouncing)
int stableState = HIGH;          // the debounced, trusted state
unsigned long lastChange = 0;    // when the raw reading last changed
int presses = 0;
bool ledOn = false;

const unsigned long LONG_PRESS_MS = 1000;
unsigned long pressStart = 0;   // when the current press began
bool resetDone = false;         // reset only once per long press

void setup() {
  pinMode(BUTTON, INPUT_PULLUP);   // built-in pull-up resistor
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  Serial.println("Press the button ONCE...");
}

void loop() {
  int reading = digitalRead(BUTTON);

  // 1. Still bouncing? Every change restarts the timer.
  if (reading != lastReading) {
    lastChange = millis();
    lastReading = reading;
  }

  // 2. Steady for 50 ms? Now we trust it.
  if (millis() - lastChange >= DEBOUNCE_MS && reading != stableState) {
    stableState = reading;

    // 3. Act only on the press (HIGH -> LOW), once.
    if (stableState == LOW) {
      presses++;
      ledOn = !ledOn;
      digitalWrite(LED, ledOn);
      Serial.print("Presses: ");
      Serial.println(presses);
      pressStart = millis();     // start timing this press
      resetDone = false;
    }
  }

  // 4. Homework: still held after 1 second? Reset the counter.
  if (stableState == LOW && !resetDone && millis() - pressStart >= LONG_PRESS_MS) {
    presses = 0;
    resetDone = true;
    Serial.println("Long press: counter reset to 0");
  }
}
