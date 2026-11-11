// Arduino in Your Browser — Episode 6: analogRead (the surprise)
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// Turn the knob slowly and watch the voltage.
// It jumps 0, 1, 2, 3, 4, 5 ... nothing in between. Why?

const int KNOB = A0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  int raw = analogRead(KNOB);        // 0 ... 1023
  int volts = raw * 5 / 1023;        // integer math!
  Serial.print("raw ");
  Serial.print(raw);
  Serial.print("  ->  ");
  Serial.print(volts);
  Serial.println(" V");
  delay(200);                        // just for printing
}
