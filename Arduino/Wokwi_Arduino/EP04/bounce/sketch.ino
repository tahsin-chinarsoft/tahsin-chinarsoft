// Arduino in Your Browser — Episode 4: Buttons Done Right (the problem)
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// Count button presses and toggle an LED on every press.
// Press once... and watch the counter jump. That's contact bounce.
// Compare with the debounced version.

const int BUTTON = 2;      // button to GND, no external resistor
const int LED = 12;

int lastReading = HIGH;    // with INPUT_PULLUP: released = HIGH
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

  if (lastReading == HIGH && reading == LOW) {   // HIGH -> LOW = pressed
    presses++;
    ledOn = !ledOn;
    digitalWrite(LED, ledOn);
    Serial.print("Presses: ");
    Serial.println(presses);
  }
  lastReading = reading;
}
