// Arduino in Your Browser — Episode 9: the WRONG way to read an NTC (straight-line map)
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// NTC module OUT -> A0. Hotter = SMALLER number.
// -24 °C reads 953, 80 °C reads 115 ... so map() between them, right?
// Click the sensor and change the temperature. Try 40 °C.

const int SENSOR = A0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  int raw = analogRead(SENSOR);
  int celsius = map(raw, 953, 115, -24, 80);   // a straight line ... wrong!

  Serial.print("raw ");
  Serial.print(raw);
  Serial.print("  ->  ");
  Serial.print(celsius);
  Serial.println(" C   (straight line)");
  delay(500);
}
