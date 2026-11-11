// ChinarSoft — Industrial ESP32 with Wokwi, Episode 1 — HOMEWORK SOLUTION
// State 4: PLANNED STOP / CHANGEOVER (yellow blinking, refused while in FAULT)
// New button: CHANGEOVER on GPIO 33 (key C). Press it to start a changeover,
// press it again (or STOP) to finish -> STOPPED, then START as usual.
//
// State codes match the ChinarSoft Virtual Machine (holding register 0):
//   0 STOPPED, 1 RUNNING, 2 IDLE, 3 FAULT, 4 CHANGEOVER
// Stop reasons match holding register 1:
//   0 none, 1 material shortage, 2 jam, 5 changeover
//
// Rules on the shop floor:
//   - START runs the machine only if material is present (otherwise IDLE).
//   - Losing material while running -> IDLE. Material back -> RUNNING again.
//   - A jam -> FAULT. A fault NEVER restarts by itself:
//     the operator presses RESET, then START.

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- pins ----------
const int PIN_START    = 25;  // green push button
const int PIN_STOP     = 26;  // red push button
const int PIN_RESET    = 27;  // blue push button
const int PIN_JAM      = 14;  // "jam sensor" push button
const int PIN_MATERIAL = 13;  // slide switch: connected to GND = material present
const int PIN_CHANGE   = 33;  // CHANGEOVER push button (homework)

const int PIN_GREEN  = 18;    // andon tower
const int PIN_YELLOW = 19;
const int PIN_RED    = 23;
const int PIN_BUZZER = 32;

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------- states ----------
enum State { STOPPED = 0, RUNNING = 1, IDLE = 2, FAULT = 3, CHANGEOVER = 4 };
enum Reason { NONE = 0, MATERIAL_SHORTAGE = 1, JAM = 2, CHANGEOVER_REASON = 5 };

const char* STATE_NAMES[]  = { "STOPPED", "RUNNING", "IDLE", "FAULT", "CHANGEOVER" };
const char* REASON_NAMES[] = { "-", "material", "jam", "", "", "changeover" };

State  state  = STOPPED;
Reason reason = NONE;
unsigned long stateSince = 0;  // millis() when we entered the current state

// ---------- buttons with a simple debounce ----------
// (Episode 2 is all about counting and debouncing properly.)
struct Button {
  int pin;
  bool stable;        // debounced level (true = HIGH)
  bool lastRead;
  unsigned long changedAt;
};

Button btnStart = { PIN_START, true, true, 0 };
Button btnStop  = { PIN_STOP,  true, true, 0 };
Button btnReset = { PIN_RESET, true, true, 0 };
Button btnJam   = { PIN_JAM,   true, true, 0 };
Button btnChange = { PIN_CHANGE, true, true, 0 };

// returns true once, at the moment the button is pressed
bool pressed(Button& b) {
  bool now = digitalRead(b.pin);
  if (now != b.lastRead) {
    b.lastRead = now;
    b.changedAt = millis();
  }
  if (millis() - b.changedAt > 30 && now != b.stable) {
    b.stable = now;
    return b.stable == LOW;   // INPUT_PULLUP: pressed = LOW
  }
  return false;
}

bool materialPresent() {
  return digitalRead(PIN_MATERIAL) == LOW;
}

// ---------- state changes ----------
void setState(State s, Reason r) {
  if (s == state && r == reason) return;
  state = s;
  reason = r;
  stateSince = millis();
  Serial.print("-> ");
  Serial.print(STATE_NAMES[state]);
  if (reason != NONE) {
    Serial.print(" (");
    Serial.print(REASON_NAMES[reason]);
    Serial.print(")");
  }
  Serial.println();
}

void updateStateMachine() {
  bool start = pressed(btnStart);
  bool stop  = pressed(btnStop);
  bool reset = pressed(btnReset);
  bool jam   = pressed(btnJam);
  bool change = pressed(btnChange);
  bool material = materialPresent();

  // homework: a changeover can start from STOPPED, RUNNING or IDLE - never from FAULT
  if (change && state == FAULT) {
    Serial.println("changeover refused: clear the fault first (RESET)");
    change = false;
  }
  if (change && state != CHANGEOVER) {
    setState(CHANGEOVER, CHANGEOVER_REASON);
    return;
  }

  switch (state) {
    case STOPPED:
      if (start) setState(material ? RUNNING : IDLE,
                          material ? NONE : MATERIAL_SHORTAGE);
      break;

    case RUNNING:
      if (stop)           setState(STOPPED, NONE);
      else if (jam)       setState(FAULT, JAM);
      else if (!material) setState(IDLE, MATERIAL_SHORTAGE);
      break;

    case IDLE:
      if (stop)          setState(STOPPED, NONE);
      else if (material) setState(RUNNING, NONE);   // waits, then resumes
      break;

    case FAULT:
      if (reset) setState(STOPPED, NONE);           // never straight to RUNNING
      break;

    case CHANGEOVER:
      if (change || stop) setState(STOPPED, NONE);  // done -> STOPPED, then START
      break;                                        // START and JAM are ignored here
  }
}

// ---------- outputs ----------
void updateAndon() {
  bool blink = (millis() / 400) % 2;   // 400 ms on / 400 ms off

  digitalWrite(PIN_GREEN,  state == RUNNING);
  digitalWrite(PIN_YELLOW, state == IDLE || (state == CHANGEOVER && blink));
  digitalWrite(PIN_RED,    state == FAULT && blink);

  // change the buzzer only when it has to change (calling tone() every loop is wasteful)
  static bool buzzing = false;
  bool wantBuzz = (state == FAULT && blink);
  if (wantBuzz != buzzing) {
    buzzing = wantBuzz;
    if (buzzing) tone(PIN_BUZZER, 880);
    else noTone(PIN_BUZZER);
  }
}

unsigned long lastReport = 0;

void report() {
  if (millis() - lastReport < 1000) return;
  lastReport = millis();
  unsigned long seconds = (millis() - stateSince) / 1000;

  // LCD: line 1 = state, line 2 = reason + time in state
  lcd.setCursor(0, 0);
  lcd.print(state == CHANGEOVER ? "" : "State: ");
  lcd.print(STATE_NAMES[state]);
  lcd.print("         ");
  lcd.setCursor(0, 1);
  lcd.print(REASON_NAMES[reason]);
  lcd.print("  ");
  lcd.print(seconds);
  lcd.print(" s          ");

  // Serial: the same data a SCADA would read (Episode 6 sends it over MQTT)
  Serial.print("{\"state\":");
  Serial.print(state);
  Serial.print(",\"name\":\"");
  Serial.print(STATE_NAMES[state]);
  Serial.print("\",\"reason\":");
  Serial.print(reason);
  Serial.print(",\"state_s\":");
  Serial.print(seconds);
  Serial.println("}");
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_START, INPUT_PULLUP);
  pinMode(PIN_STOP, INPUT_PULLUP);
  pinMode(PIN_RESET, INPUT_PULLUP);
  pinMode(PIN_JAM, INPUT_PULLUP);
  pinMode(PIN_MATERIAL, INPUT_PULLUP);
  pinMode(PIN_CHANGE, INPUT_PULLUP);

  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_YELLOW, OUTPUT);
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  lcd.init();
  lcd.backlight();

  Serial.println("ChinarSoft machine ready. Press START. C = changeover.");
  stateSince = millis();
}

void loop() {
  updateStateMachine();
  updateAndon();
  report();
  delay(5);
}
