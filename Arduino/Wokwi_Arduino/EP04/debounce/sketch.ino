// Arduino in Your Browser — Episode 4: Buttons Done Right (the fix)
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
    }
  }
}
