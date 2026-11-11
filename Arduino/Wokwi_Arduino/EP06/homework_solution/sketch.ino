// Arduino in Your Browser — Episode 6 HOMEWORK SOLUTION: LED bar graph
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// Turn the knob: 0 to 10 segments light up.
// Bar graph segments on pins 2 ... 11, knob on A0.

const int KNOB = A0;
const int FIRST_PIN = 2;   // segment 1 = pin 2, segment 10 = pin 11
const int SEGMENTS = 10;

int lastLit = -1;

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < SEGMENTS; i++) {
    pinMode(FIRST_PIN + i, OUTPUT);
  }
}

void loop() {
  int raw = analogRead(KNOB);                // 0 ... 1023
  int lit = map(raw, 0, 1023, 0, SEGMENTS);  // 0 ... 10

  for (int i = 0; i < SEGMENTS; i++) {
    digitalWrite(FIRST_PIN + i, i < lit ? HIGH : LOW);
  }

  if (lit != lastLit) {                      // print only when it changes
    lastLit = lit;
    Serial.print("raw ");
    Serial.print(raw);
    Serial.print("  ->  ");
    Serial.print(lit);
    Serial.println(" / 10");
  }
}
