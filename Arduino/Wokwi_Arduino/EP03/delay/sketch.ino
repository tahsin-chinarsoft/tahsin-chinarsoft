// Arduino in Your Browser — Episode 3: Traffic Light with delay() (the problem)
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// This version works... until you press the pedestrian button.
// During delay() the Arduino does nothing else: the button is only
// read for an instant once per cycle, and the heartbeat LED freezes.
// Compare with the millis() version.

const int RED = 12, YELLOW = 11, GREEN = 10;
const int BUTTON = 2;                       // pedestrian button

void lights(bool r, bool y, bool g) {
  digitalWrite(RED, r);
  digitalWrite(YELLOW, y);
  digitalWrite(GREEN, g);
}

void setup() {
  pinMode(RED, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  Serial.begin(9600);
}

void loop() {
  lights(0, 0, 1);
  Serial.println("GREEN");
  delay(5000);                              // stuck here for 5 seconds

  // the only moment the button is read:
  if (digitalRead(BUTTON) == LOW) Serial.println("Button seen");

  lights(0, 1, 0);
  Serial.println("YELLOW");
  delay(1500);                              // stuck again

  lights(1, 0, 0);
  Serial.println("RED");
  delay(4000);                              // and again

  // heartbeat: can only change once per cycle
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}
