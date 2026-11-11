// ChinarSoft — Industrial ESP32 with Wokwi, Episode 4
// Relays and interlocks: the machine finally switches real loads.
//
// Same machine as Episodes 1-3 (andon, START / STOP / RESET / JAM, material, PART counter,
// motor temperature NTC on GPIO 34, air pressure 4-20 mA on GPIO 35 - filter + hysteresis on)
// plus:
//   K1 MOTOR relay  on GPIO 4   (relay module, COM-NO switches the motor lamp)
//   K2 CLAMP relay  on GPIO 17  (relay module, COM-NO switches the clamp valve lamp)
//   GUARD DOOR switch on GPIO 16 (slide switch: connected to GND = door closed)
//
// Sequence:  START -> clamp closes first, motor 300 ms later.
//            STOP  -> motor off at once, clamp opens 500 ms later.
// Interlock: the motor relay is decided every loop by motorAllowed(), not by a button.
// Guard:     opening the door stops the machine; closing it NEVER restarts it.
//
// NOTE: this is production logic, not a safety function. A real guard door goes through a
// safety relay that cuts the contactor power in hardware (ISO 13849 / EN 60204-1).
//
// State codes = ChinarSoft Virtual Machine HR0: 0 STOPPED, 1 RUNNING, 2 IDLE, 3 FAULT
// Reasons     = HR1: 0 none, 1 material shortage, 2 jam, 3 motor overload, 4 sensor fault,
//               9 guard open (our own code - the Virtual Machine list stops at 7)

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

// ---------- pins ----------
const int PIN_START    = 25;
const int PIN_STOP     = 26;
const int PIN_RESET    = 27;
const int PIN_JAM      = 14;
const int PIN_MATERIAL = 13;  // slide switch: connected to GND = material present
const int PIN_PART     = 33;  // part sensor (push button "PART", key P)
const int PIN_TEMP     = 34;  // NTC, ADC1
const int PIN_PRESSURE = 35;  // 4-20 mA across 165 ohm, ADC1
const int PIN_DOOR     = 16;  // guard door switch: connected to GND = closed

const int PIN_K1_MOTOR = 4;   // relay inputs: pins that stay LOW while the ESP32 boots
const int PIN_K2_CLAMP = 17;  // (avoid the strapping pins 0, 2, 5, 12, 15)

const int PIN_GREEN  = 18;
const int PIN_YELLOW = 19;
const int PIN_RED    = 23;
const int PIN_BUZZER = 32;

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------- limits ----------
const float TEMP_WARN_ON  = 60.0;
const float TEMP_WARN_OFF = 55.0;
const float TEMP_TRIP     = 75.0;
const float TEMP_RESET_OK = 70.0;
const float MA_MIN        = 3.6;
const float MA_MAX        = 21.0;
const float BAR_FULL      = 10.0;
const float SHUNT_OHM     = 165.0;
const float FILTER_ALPHA  = 0.1;

const unsigned long CLAMP_SETTLE_MS = 300;  // clamp closes before the motor starts
const unsigned long MOTOR_COAST_MS  = 500;  // motor stops before the clamp opens

// ---------- states ----------
enum State { STOPPED = 0, RUNNING = 1, IDLE = 2, FAULT = 3 };
enum Reason { NONE = 0, MATERIAL_SHORTAGE = 1, JAM = 2, MOTOR_OVERLOAD = 3, SENSOR_FAULT = 4,
              GUARD_OPEN = 9 };

const char* STATE_NAMES[]  = { "STOPPED", "RUNNING", "IDLE", "FAULT" };
const char* REASON_NAMES[] = { "-", "material", "jam", "motor overtemp", "sensor fault",
                               "", "", "", "", "guard open" };
const char* REASON_SHORT[] = { "", " MAT", " JAM", " OVT", " SNS", "", "", "", "", " GRD" };

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

// ---------- sensor values (Episode 3) ----------
float tempC = 25.0;
float mA = 12.0;
float bar = 5.0;
bool tempBroken = false;
bool loopBroken = false;
bool hot = false;

// ---------- relays ----------
bool k1Motor = false;
bool k2Clamp = false;
unsigned long clampOnSince = 0;   // when K2 closed
unsigned long motorOffSince = 0;  // when K1 opened
const char* block = "-";          // first interlock that holds the motor off

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

bool materialPresent() { return digitalRead(PIN_MATERIAL) == LOW; }
bool doorClosed()      { return digitalRead(PIN_DOOR) == LOW; }
bool sensorFault()     { return tempBroken || loopBroken; }

// ---------- part counting (Episode 2) ----------
const unsigned long QUIET_US = 20000;
volatile uint32_t rawEdges = 0;
volatile unsigned long lastEdgeUs = 0;

void IRAM_ATTR onPartChange() {
  if (digitalRead(PIN_PART) == LOW) rawEdges++;
  lastEdgeUs = micros();
}

bool partStable = HIGH;
uint32_t goodCount = 0;
unsigned long lastPartMs = 0;
unsigned int cycleDs = 0;

void setState(State s, Reason r) {
  if (s == state && r == reason) return;
  if (s != RUNNING) lastPartMs = 0;
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

// ---------- sensors every 50 ms (Episode 3, filter + hysteresis always on) ----------
unsigned long lastSample = 0;

void readSensors() {
  if (millis() - lastSample < 50) return;
  lastSample = millis();

  int t = analogRead(PIN_TEMP);
  tempBroken = (t < 20 || t > 4075);
  if (!tempBroken) {
    const float BETA = 3950;
    float raw = 1 / (log(1 / (4095. / t - 1)) / BETA + 1.0 / 298.15) - 273.15;
    tempC += FILTER_ALPHA * (raw - tempC);
    if (!hot && tempC >= TEMP_WARN_ON) hot = true;
    if (hot && tempC < TEMP_WARN_OFF) hot = false;
  }

  int p = analogRead(PIN_PRESSURE);
  mA = p * 3.3 / 4095.0 / SHUNT_OHM * 1000.0;
  loopBroken = (mA < MA_MIN || mA > MA_MAX);
  bar = (mA - 4.0) * BAR_FULL / 16.0;
  if (bar < 0) bar = 0;
}

void updateStateMachine() {
  bool start = pressed(btnStart);
  bool stop  = pressed(btnStop);
  bool reset = pressed(btnReset);
  bool jam   = pressed(btnJam);
  bool material = materialPresent();

  // protections first: they beat any button
  if (state == RUNNING || state == IDLE) {
    if (sensorFault())      { setState(FAULT, SENSOR_FAULT);   return; }
    if (tempC >= TEMP_TRIP) { setState(FAULT, MOTOR_OVERLOAD); return; }
    if (!doorClosed())      { setState(STOPPED, GUARD_OPEN);   return; }  // no restart on close
  }

  switch (state) {
    case STOPPED:
      if (start) {
        if (!doorClosed())               Serial.println("start refused: guard door open");
        else if (sensorFault())          Serial.println("start refused: sensor fault");
        else if (tempC >= TEMP_RESET_OK) Serial.println("start refused: motor too hot");
        else setState(material ? RUNNING : IDLE, material ? NONE : MATERIAL_SHORTAGE);
      }
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
      if (reset) {
        if (sensorFault())               Serial.println("reset refused: sensor fault");
        else if (tempC >= TEMP_RESET_OK) Serial.println("reset refused: motor too hot");
        else setState(STOPPED, NONE);
      }
      break;
  }
}

// ---------- the interlock layer: checked every loop, not only when a button is pressed ----------
bool motorAllowed() {
  if (state != RUNNING)                         { block = "state";  return false; }
  if (!doorClosed())                            { block = "door";   return false; }
  if (sensorFault())                            { block = "sensor"; return false; }
  if (tempC >= TEMP_TRIP)                       { block = "temp";   return false; }
  if (!k2Clamp)                                 { block = "clamp";  return false; }
  if (millis() - clampOnSince < CLAMP_SETTLE_MS) { block = "clamp";  return false; }
  block = "-";
  return true;
}

void setRelay(int pin, bool& now, bool want, const char* name) {
  if (want == now) return;
  now = want;
  digitalWrite(pin, want ? HIGH : LOW);
  Serial.print(name);
  Serial.println(want ? " ON" : " OFF");
}

void updateRelays() {
  unsigned long t = millis();

  // K1 motor: on only while every interlock agrees, off at once otherwise
  bool wasMotor = k1Motor;
  setRelay(PIN_K1_MOTOR, k1Motor, motorAllowed(), "K1 MOTOR");
  if (wasMotor && !k1Motor) motorOffSince = t;

  // K2 clamp: closes when RUNNING, opens only after the motor has been off for 500 ms
  bool wantClamp;
  if (state == RUNNING && doorClosed()) wantClamp = true;
  else if (k1Motor) wantClamp = true;
  else if (k2Clamp && t - motorOffSince < MOTOR_COAST_MS) wantClamp = true;
  else wantClamp = false;
  if (wantClamp && !k2Clamp) clampOnSince = t;
  setRelay(PIN_K2_CLAMP, k2Clamp, wantClamp, "K2 CLAMP");
}

// ---------- count parts (Episode 2) ----------
void updateCounter() {
  if (micros() - lastEdgeUs < QUIET_US) return;
  bool level = digitalRead(PIN_PART);
  if (level == partStable) return;
  partStable = level;
  if (level == LOW && state == RUNNING && k1Motor) {   // no motor, no part
    unsigned long now = millis();
    if (lastPartMs != 0) cycleDs = (now - lastPartMs + 50) / 100;
    lastPartMs = now;
    goodCount++;
  }
}

// ---------- outputs ----------
void updateAndon() {
  bool blink = (millis() / 400) % 2;
  digitalWrite(PIN_GREEN,  state == RUNNING);
  digitalWrite(PIN_YELLOW, state == IDLE || (state == STOPPED && reason == GUARD_OPEN && blink));
  digitalWrite(PIN_RED,    state == FAULT && blink);

  static bool buzzing = false;
  bool wantBuzz = (state == FAULT && blink);
  if (wantBuzz != buzzing) {
    buzzing = wantBuzz;
    if (buzzing) tone(PIN_BUZZER, 880);
    else noTone(PIN_BUZZER);
  }
}

void serial1(float v) {
  int x = (int)(v * 10 + (v < 0 ? -0.5 : 0.5));
  if (x < 0) { Serial.print("-"); x = -x; }
  Serial.print(x / 10);
  Serial.print(".");
  Serial.print(x % 10);
}

unsigned long lastReport = 0;

void report() {
  if (millis() - lastReport < 500) return;
  lastReport = millis();

  // LCD line 1: state + reason (+ DOOR when open), line 2: relays
  lcd.setCursor(0, 0);
  lcd.print(STATE_NAMES[state]);
  lcd.print(REASON_SHORT[reason]);
  if (!doorClosed() && reason != GUARD_OPEN) lcd.print(" DOOR");
  lcd.print("        ");
  lcd.setCursor(0, 1);
  lcd.print(k1Motor ? "MOT:ON  " : "MOT:off ");
  lcd.print(k2Clamp ? "CLMP:ON " : "CLMP:off");

  Serial.print("{\"state\":");
  Serial.print(state);
  Serial.print(",\"reason\":");
  Serial.print(reason);
  Serial.print(",\"motor\":");
  Serial.print(k1Motor ? 1 : 0);
  Serial.print(",\"clamp\":");
  Serial.print(k2Clamp ? 1 : 0);
  Serial.print(",\"door\":");
  Serial.print(doorClosed() ? 1 : 0);
  Serial.print(",\"block\":\"");
  Serial.print(block);
  Serial.print("\",\"temp_c\":");
  serial1(tempC);
  Serial.print(",\"bar\":");
  serial1(bar);
  Serial.print(",\"good\":");
  Serial.print(goodCount);
  Serial.println("}");
}

void setup() {
  // relays first: write LOW before the pin becomes an output, so they never click at boot
  digitalWrite(PIN_K1_MOTOR, LOW);
  digitalWrite(PIN_K2_CLAMP, LOW);
  pinMode(PIN_K1_MOTOR, OUTPUT);
  pinMode(PIN_K2_CLAMP, OUTPUT);

  Serial.begin(115200);

  pinMode(PIN_START, INPUT_PULLUP);
  pinMode(PIN_STOP, INPUT_PULLUP);
  pinMode(PIN_RESET, INPUT_PULLUP);
  pinMode(PIN_JAM, INPUT_PULLUP);
  pinMode(PIN_MATERIAL, INPUT_PULLUP);
  pinMode(PIN_PART, INPUT_PULLUP);
  pinMode(PIN_DOOR, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_PART), onPartChange, CHANGE);

  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_YELLOW, OUTPUT);
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  analogReadResolution(12);
  lcd.init();
  lcd.backlight();

  // start the temperature filter at the real value
  int t = analogRead(PIN_TEMP);
  if (t > 20 && t < 4075) tempC = 1 / (log(1 / (4095. / t - 1)) / 3950 + 1.0 / 298.15) - 273.15;

  Serial.println("ChinarSoft machine ready. Close the guard door (D), then START.");
  stateSince = millis();
}

void loop() {
  readSensors();
  updateStateMachine();
  updateRelays();
  updateCounter();
  updateAndon();
  report();
  delay(5);
}
