// ChinarSoft - Arduino Sensor Kit, Episode 12 (Season 1 finale)
// Relay module (KY-019) + clap microphone + photoresistor + button + reed switch
// Project: Smart lamp
//   - Double clap or button press toggles the lamp
//   - Door opens (reed) while the room is dark -> lamp on automatically
//   - Lamp turns itself off after AUTO_OFF_MS
//   - Relay protection: never switch faster than MIN_SWITCH_MS
//
// LOW-VOLTAGE LOAD ONLY ON CAMERA (e.g. 12 V LED strip with its own adapter).
//
// Wiring (Arduino Uno):
//   KY-019 relay      S -> D7 | + -> 5V | - -> GND | COM + NO in series with the 12 V lamp
//   KY-037 mic        AO -> A0
//   KY-018 LDR        S -> A1
//   KY-004 button     S -> D2
//   KY-021 reed       S -> D4 (magnet on the door, door CLOSED at boot)

const int RELAY_PIN = 7;
const int MIC_PIN = A0;
const int LDR_PIN = A1;
const int BUTTON_PIN = 2;
const int REED_PIN = 4;

const int CLAP_THRESHOLD = 120;
const int DARK_ON  = 300;     // LDR below this = dark   (hysteresis:
const int DARK_OFF = 380;     // must rise above this to count as bright again)
const unsigned long AUTO_OFF_MS   = 10UL * 60UL * 1000UL;   // 10 minutes
const unsigned long MIN_SWITCH_MS = 500;

bool lampOn = false;
bool isDark = false;
unsigned long lampOnSince = 0;
unsigned long lastSwitch = 0;
int buttonIdle, reedIdle;

void setLamp(bool on, const char *reason) {
  if (on == lampOn) return;
  if (millis() - lastSwitch < MIN_SWITCH_MS) return;   // protect the relay contacts
  lampOn = on;
  lastSwitch = millis();
  if (on) lampOnSince = millis();
  digitalWrite(RELAY_PIN, on ? HIGH : LOW);
  Serial.print(on ? "Lamp ON  (" : "Lamp OFF (");
  Serial.print(reason);
  Serial.println(")");
}

int micPeakToPeak() {
  int lo = 1023, hi = 0;
  unsigned long t = millis();
  while (millis() - t < 20) {
    int v = analogRead(MIC_PIN);
    if (v < lo) lo = v;
    if (v > hi) hi = v;
  }
  return hi - lo;
}

bool doubleClap() {
  static unsigned long first = 0, last = 0;
  unsigned long now = millis();
  if (micPeakToPeak() > CLAP_THRESHOLD && now - last > 100) {
    last = now;
    if (first && now - first > 150 && now - first < 700) { first = 0; return true; }
    first = now;
  }
  if (first && now - first > 700) first = 0;
  return false;
}

void setup() {
  Serial.begin(9600);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(REED_PIN, INPUT_PULLUP);
  delay(100);
  buttonIdle = digitalRead(BUTTON_PIN);
  reedIdle = digitalRead(REED_PIN);
  Serial.println("Smart lamp ready.");
}

void loop() {
  // Light level with hysteresis, so the lamp's own light doesn't confuse us
  int light = analogRead(LDR_PIN);
  if (!isDark && light < DARK_ON) isDark = true;
  if (isDark && light > DARK_OFF) isDark = false;
  // NOTE: if your KY-018 reads HIGHER in the dark, swap the comparisons.

  if (doubleClap()) setLamp(!lampOn, "double clap");

  static int lastButton = buttonIdle;
  int b = digitalRead(BUTTON_PIN);
  if (b != buttonIdle && lastButton == buttonIdle) setLamp(!lampOn, "button");
  lastButton = b;

  static bool lastDoorOpen = false;
  bool doorOpen = digitalRead(REED_PIN) != reedIdle;
  if (doorOpen && !lastDoorOpen && isDark) setLamp(true, "door opened in the dark");
  lastDoorOpen = doorOpen;

  if (lampOn && millis() - lampOnSince > AUTO_OFF_MS) setLamp(false, "auto-off timer");
}
