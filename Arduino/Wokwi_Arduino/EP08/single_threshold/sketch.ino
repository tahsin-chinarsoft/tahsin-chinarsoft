// Arduino in Your Browser — Episode 8: night lamp, ONE threshold (it flickers at dusk)
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// Photoresistor module: AO -> A0. More light = SMALLER number.
// Click the sensor and move the lux slider.

const int SENSOR = A0;
const int LAMP = 9;
const int DARK = 600;          // above this = dark

bool lampOn = false;

void setup() {
  Serial.begin(9600);
  pinMode(LAMP, OUTPUT);
}

void loop() {
  int light = analogRead(SENSOR);   // bright ~170, dark ~850

  bool newState = (light > DARK);
  if (newState != lampOn) {
    lampOn = newState;
    digitalWrite(LAMP, lampOn);
    Serial.print("light ");
    Serial.print(light);
    Serial.println(lampOn ? "  -> LAMP ON" : "  -> LAMP OFF");
  }
  delay(50);
}
