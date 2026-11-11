// Arduino in Your Browser — Episode 5: Serial Monitor (the surprise)
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// Type 5 in the Serial Monitor and press Enter.
// The Arduino answers 53... and then 10. Why?
// Serial.read() gives you ONE character code at a time.

void setup() {
  Serial.begin(9600);
  Serial.println("Type something and press Enter:");
}

void loop() {
  if (Serial.available() > 0) {     // is there anything to read?
    int c = Serial.read();          // read ONE character
    Serial.print("I got: ");
    Serial.println(c);              // print its number (ASCII code)
  }
}
