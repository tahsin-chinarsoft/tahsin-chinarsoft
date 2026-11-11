// Arduino in Your Browser — Episode 8: night lamp with HYSTERESIS (no flicker)
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// Two thresholds: switch ON when it gets really dark,
// switch OFF only when it gets clearly brighter. In between: keep the state.

const int SENSOR = A0;
const int LAMP = 9;
const int ON_ABOVE  = 650;     // darker than this -> on
const int OFF_BELOW = 550;     // brighter than this -> off

bool lampOn = false;

void setup() {
  Serial.begin(9600);
  pinMode(LAMP, OUTPUT);
}

void loop() {
  int light = analogRead(SENSOR);

  if (!lampOn && light > ON_ABOVE) {
    lampOn = true;
    digitalWrite(LAMP, HIGH);
    Serial.print("light "); Serial.print(light); Serial.println("  -> LAMP ON");
  }
  else if (lampOn && light < OFF_BELOW) {
    lampOn = false;
    digitalWrite(LAMP, LOW);
    Serial.print("light "); Serial.print(light); Serial.println("  -> LAMP OFF");
  }
  delay(50);
}
