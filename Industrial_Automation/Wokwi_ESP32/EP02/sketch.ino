// ChinarSoft — Industrial ESP32 with Wokwi, Episode 2
// Counting parts without missing one (and without counting one twice)
//
// Same machine as Episode 1 (andon, START / STOP / RESET / JAM, material switch)
// plus a PART sensor. Wokwi push buttons bounce like real contacts, so:
//   raw  = every falling edge the interrupt sees  (lies)
//   good = HIGH->LOW changes after the pin is quiet for 20 ms, only while RUNNING (truth)
// Cycle time = time between two good parts, 0.1 s units like the Virtual Machine.
//
// State codes = ChinarSoft Virtual Machine HR0: 0 STOPPED, 1 RUNNING, 2 IDLE, 3 FAULT
// Reasons     = HR1: 0 none, 1 material shortage, 2 jam

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- pins ----------
const int PIN_START    = 25;
const int PIN_STOP     = 26;
const int PIN_RESET    = 27;
const int PIN_JAM      = 14;
const int PIN_MATERIAL = 13;  // slide switch: connected to GND = material present
const int PIN_PART     = 33;  // part sensor (push button "PART", key P)

const int PIN_GREEN  = 18;
const int PIN_YELLOW = 19;
const int PIN_RED    = 23;
const int PIN_BUZZER = 32;

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------- states ----------
enum State { STOPPED = 0, RUNNING = 1, IDLE = 2, FAULT = 3 };
enum Reason { NONE = 0, MATERIAL_SHORTAGE = 1, JAM = 2 };

const char* STATE_NAMES[]  = { "STOPPED", "RUNNING", "IDLE", "FAULT" };
const char* REASON_NAMES[] = { "-", "material", "jam" };

State  state  = STOPPED;
Reason reason = NONE;
unsigned long stateSince = 0;

// ---------- buttons with a simple debounce (Episode 1) ----------
struct Button {
  int pin;
  bool stable;
  bool lastRead;
  unsigned long changedAt;
};

Button btnStart = { PIN_START, true, true, 0 };
Button btnStop  = { PIN_STOP,  true, true, 0 };
Button btnReset = { PIN_RESET, true, true, 0 };
Button btnJam   = { PIN_JAM,   true, true, 0 };

bool pressed(Button& b) {
  bool now = digitalRead(b.pin);
  if (now != b.lastRead) {
    b.lastRead = now;
    b.changedAt = millis();
  }
  if (millis() - b.changedAt > 30 && now != b.stable) {
    b.stable = now;
    return b.stable == LOW;
  }
  return false;
}

bool materialPresent() {
  return digitalRead(PIN_MATERIAL) == LOW;
}

// ---------- part counting (interrupt) ----------
const unsigned long QUIET_US = 20000;   // an edge counts only after 20 ms of silence

volatile uint32_t rawEdges = 0;         // what a naive interrupt counter sees
volatile unsigned long lastEdgeUs = 0;  // time of the last edge, rising or falling

// The interrupt does almost nothing: count the edge, remember WHEN it happened.
// Deciding inside the interrupt is a trap: while the contact bounces, the pin may
// already read HIGH again when we look at it.
void IRAM_ATTR onPartChange() {
  if (digitalRead(PIN_PART) == LOW) rawEdges++;
  lastEdgeUs = micros();                // every edge restarts the quiet timer
}

bool partStable = HIGH;                 // debounced level of the sensor
uint32_t goodCount = 0;                 // parts made while RUNNING
unsigned long lastPartMs = 0;           // 0 = no part yet in this run
unsigned int cycleDs = 0;               // last cycle time, 0.1 s

void setState(State s, Reason r) {
  if (s == state && r == reason) return;
  if (s != RUNNING) lastPartMs = 0;     // a stop is not a slow cycle
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
  bool material = materialPresent();

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
      else if (material) setState(RUNNING, NONE);
      break;
    case FAULT:
      if (reset) setState(STOPPED, NONE);
      break;
  }
}

// ---------- count parts ----------
void updateCounter() {
  // wait until the pin has been quiet for 20 ms, then believe it
  if (micros() - lastEdgeUs < QUIET_US) return;
  bool level = digitalRead(PIN_PART);
  if (level == partStable) return;
  partStable = level;

  if (level == LOW) {                   // sensor activated = one part
    if (state != RUNNING) return;       // a hand in front of the sensor is not a part
    unsigned long now = millis();
    if (lastPartMs != 0) cycleDs = (now - lastPartMs + 50) / 100;
    lastPartMs = now;
    goodCount++;
    Serial.print("part #");
    Serial.print(goodCount);
    Serial.print("  cycle ");
    Serial.print(cycleDs / 10);
    Serial.print(".");
    Serial.print(cycleDs % 10);
    Serial.println(" s");
  }
}

// ---------- outputs ----------
void updateAndon() {
  bool blink = (millis() / 400) % 2;
  digitalWrite(PIN_GREEN,  state == RUNNING);
  digitalWrite(PIN_YELLOW, state == IDLE);
  digitalWrite(PIN_RED,    state == FAULT && blink);

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

  noInterrupts();
  uint32_t raw = rawEdges;
  interrupts();

  // LCD line 1: state + good parts, line 2: raw edges + cycle time
  lcd.setCursor(0, 0);
  lcd.print(STATE_NAMES[state]);
  lcd.print("  good ");
  lcd.print(goodCount);
  lcd.print("     ");
  lcd.setCursor(0, 1);
  lcd.print("raw ");
  lcd.print(raw);
  lcd.print(" cyc ");
  lcd.print(cycleDs / 10);
  lcd.print(".");
  lcd.print(cycleDs % 10);
  lcd.print("s   ");

  Serial.print("{\"state\":");
  Serial.print(state);
  Serial.print(",\"reason\":");
  Serial.print(reason);
  Serial.print(",\"good\":");
  Serial.print(goodCount);
  Serial.print(",\"raw\":");
  Serial.print(raw);
  Serial.print(",\"cycle_ds\":");
  Serial.print(cycleDs);
  Serial.println("}");
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_START, INPUT_PULLUP);
  pinMode(PIN_STOP, INPUT_PULLUP);
  pinMode(PIN_RESET, INPUT_PULLUP);
  pinMode(PIN_JAM, INPUT_PULLUP);
  pinMode(PIN_MATERIAL, INPUT_PULLUP);
  pinMode(PIN_PART, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_PART), onPartChange, CHANGE);

  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_YELLOW, OUTPUT);
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  lcd.init();
  lcd.backlight();

  Serial.println("ChinarSoft machine ready. Press START, then feed parts (P).");
  stateSince = millis();
}

void loop() {
  updateStateMachine();
  updateCounter();
  updateAndon();
  report();
  delay(5);
}
