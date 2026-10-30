// ChinarSoft - Arduino Sensor Kit, Episode 12 (Season 1 finale) - TASK SOLUTION
// Task: a long press on the button (over 2 s) switches the door automation on or off.
//
// What changed vs E12_SmartLamp:
//   - New flag doorAuto (starts ON). "Door opens in the dark -> lamp on" only runs
//     while doorAuto is true.
//   - The button is now handled on RELEASE: we remember when it was pressed, and on
//     release check how long it was held:
//       held > 2000 ms -> doorAuto = !doorAuto
//       shorter        -> setLamp(!lampOn, "button")   (same as before)
//     So a short press now acts when you let go, not when you push.
//   - 50 ms debounce on the button edges, so contact bounce can't fake a short press.
//   - Double clap, light sensing with hysteresis, auto-off timer and relay protection unchanged.
//
// LOW-VOLTAGE LOAD ONLY ON CAMERA (e.g. 12 V LED strip with its own adapter).
//
// Wiring: same wiring as E12_SmartLamp (no extra parts):
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
const unsigned long LONG_PRESS_MS = 2000;   // TASK: longer than this = long press
const unsigned long DEBOUNCE_MS   = 50;     // TASK

bool lampOn = false;
bool isDark = false;
bool doorAuto = true;         // TASK: door automation on/off
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
  Serial.println("Smart lamp ready. Door automation ON (hold the button > 2 s to toggle).");
}

void loop() {
  // Light level with hysteresis, so the lamp's own light doesn't confuse us
  int light = analogRead(LDR_PIN);
  if (!isDark && light < DARK_ON) isDark = true;
  if (isDark && light > DARK_OFF) isDark = false;
  // NOTE: if your KY-018 reads HIGHER in the dark, swap the comparisons.

  if (doubleClap()) setLamp(!lampOn, "double clap");

  // TASK: button with press timing - act on release
  static bool lastDown = false;
  static unsigned long pressStart = 0, lastEdge = 0;
  bool down = digitalRead(BUTTON_PIN) != buttonIdle;
  if (down != lastDown && millis() - lastEdge > DEBOUNCE_MS) {
    lastEdge = millis();
    lastDown = down;
    if (down) {
      pressStart = millis();                       // pressed: start timing
    } else {
      unsigned long held = millis() - pressStart;  // on release: how long was it held?
      if (held > LONG_PRESS_MS) {
        doorAuto = !doorAuto;
        Serial.println(doorAuto ? "Door automation ON" : "Door automation OFF");
      } else {
        setLamp(!lampOn, "button");
      }
    }
  }

  static bool lastDoorOpen = false;
  bool doorOpen = digitalRead(REED_PIN) != reedIdle;
  if (doorAuto && doorOpen && !lastDoorOpen && isDark)   // TASK: only when automation is on
    setLamp(true, "door opened in the dark");
  lastDoorOpen = doorOpen;

  if (lampOn && millis() - lampOnSince > AUTO_OFF_MS) setLamp(false, "auto-off timer");
}
