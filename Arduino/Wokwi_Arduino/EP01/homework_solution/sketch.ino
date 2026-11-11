// Episode 1 homework solution: SOS in Morse code
// ChinarSoft | Arduino in Your Browser
// S = 3 short, O = 3 long. Same circuit as Episode 1.

const int EXTERNAL_LED = 12;
const int UNIT = 200;          // one Morse "unit" in milliseconds

void flash(int units) {
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(EXTERNAL_LED, HIGH);
  delay(units * UNIT);
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(EXTERNAL_LED, LOW);
  delay(UNIT);                 // gap between flashes = 1 unit
}

void letter(int units) {       // three flashes of the same length
  for (int i = 0; i < 3; i++) flash(units);
  delay(2 * UNIT);             // gap between letters = 3 units total
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(EXTERNAL_LED, OUTPUT);
}

void loop() {
  letter(1);                   // S  . . .
  letter(3);                   // O  - - -
  letter(1);                   // S  . . .
  delay(4 * UNIT);             // gap between words = 7 units total
}
