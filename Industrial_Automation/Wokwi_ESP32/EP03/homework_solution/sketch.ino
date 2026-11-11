// ChinarSoft — Industrial ESP32 with Wokwi, Episode 3 — HOMEWORK SOLUTION
// Low air pressure interlock: below 4.0 bar for 2 s -> IDLE (reason 8, low air).
// Back to RUNNING only above 4.5 bar (hysteresis) and with material present.
// Reason 8 is our own code - the ChinarSoft Virtual Machine list stops at 7.
// Sensors that matter: motor temperature (NTC) and air pressure (4-20 mA)
//
// Same machine as Episodes 1-2 (andon, START / STOP / RESET / JAM, material, PART counter)
// plus two analog sensors:
//   NTC on GPIO 34    -> motor temperature. Warning (HOT) with hysteresis, trip at 75 C.
//   4-20 mA on GPIO 35 -> air pressure 0-10 bar. In Wokwi a potentiometer stands in for
//                        the transmitter: 0-3.3 V = the voltage across a 165 ohm shunt
//                        (4 mA = 0.66 V, 20 mA = 3.30 V). Below 3.6 mA = broken wire.
// Two slide switches let you see the problem and the fix:
//   NOISE  (GPIO 16): adds +/-2 C of electrical noise, like a VFD next to the cable
//   SMART  (GPIO 17): ON = filter + hysteresis, OFF = naive "temp >= 60" alarm
//
// State codes = ChinarSoft Virtual Machine HR0: 0 STOPPED, 1 RUNNING, 2 IDLE, 3 FAULT
// Reasons     = HR1: 0 none, 1 material shortage, 2 jam, 3 motor overload, 4 sensor fault

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
const int PIN_TEMP     = 34;  // NTC, ADC1 (ADC2 pins stop working once WiFi is on)
const int PIN_PRESSURE = 35;  // 4-20 mA across 165 ohm, ADC1
const int PIN_NOISE    = 16;  // slide switch: GND = inject noise
const int PIN_SMART    = 17;  // slide switch: GND = filter + hysteresis ON

const int PIN_GREEN  = 18;
const int PIN_YELLOW = 19;
const int PIN_RED    = 23;
const int PIN_BUZZER = 32;

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------- limits ----------
const float TEMP_WARN_ON  = 60.0;  // HOT warning switches on at 60 C ...
const float TEMP_WARN_OFF = 55.0;  // ... and off only below 55 C (hysteresis)
const float TEMP_TRIP     = 75.0;  // motor overload trip, needs RESET
const float TEMP_RESET_OK = 70.0;  // RESET refused while hotter than this
const float MA_MIN        = 3.6;   // NAMUR NE 43: below 3.6 mA = wire break
const float MA_MAX        = 21.0;  //               above 21 mA = short / failure
const float BAR_FULL      = 10.0;  // 4 mA = 0 bar, 20 mA = 10 bar
const float SHUNT_OHM     = 165.0;
const float FILTER_ALPHA  = 0.1;   // new = old + 0.1 * (raw - old), every 50 ms
const float AIR_LOW_BAR   = 4.0;   // homework: below this ...
const unsigned long AIR_LOW_MS = 2000;  // ... for this long -> IDLE
const float AIR_OK_BAR    = 4.5;   // back to RUNNING only above this

// ---------- states ----------
enum State { STOPPED = 0, RUNNING = 1, IDLE = 2, FAULT = 3 };
enum Reason { NONE = 0, MATERIAL_SHORTAGE = 1, JAM = 2, MOTOR_OVERLOAD = 3, SENSOR_FAULT = 4, LOW_AIR = 8 };

const char* STATE_NAMES[]  = { "STOPPED", "RUNNING", "IDLE", "FAULT" };
const char* REASON_NAMES[] = { "-", "material", "jam", "motor overtemp", "sensor fault", "", "", "", "low air" };
const char* REASON_SHORT[] = { "    ", " MAT", " JAM", " OVT", " SNS", "", "", "", " AIR" };

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

// ---------- sensor values ----------
float tempRaw = 25.0;      // what the ADC says right now (noise included)
float tempC = 25.0;        // what the machine decides with
float mA = 12.0;           // loop current
float bar = 5.0;           // scaled pressure
bool tempBroken = false;   // NTC open / shorted
bool loopBroken = false;   // 4-20 mA out of range
bool hot = false;          // HOT warning
uint32_t alarmCount = 0;   // how many times HOT switched on
unsigned long chirpUntil = 0;
bool lowAir = false;       // homework: air pressure interlock active
unsigned long airLowSince = 0;  // 0 = pressure is not below 4.0 bar

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
bool smartMode()       { return digitalRead(PIN_SMART) == LOW; }
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

// ---------- read the sensors every 50 ms ----------
uint32_t noiseSeed = 12345;
float noise1C() {                     // pseudo-random -1.0 .. +1.0 C
  noiseSeed = noiseSeed * 1103515245UL + 12345UL;
  return ((noiseSeed >> 16) % 201) / 100.0 - 1.0;
}
float noise2C() { return noise1C() + noise1C(); }   // -2 .. +2 C, mostly near 0

unsigned long lastSample = 0;

void readSensors() {
  if (millis() - lastSample < 50) return;
  lastSample = millis();

  // NTC: 10k thermistor + 10k resistor, beta 3950 (Wokwi part)
  int t = analogRead(PIN_TEMP);
  tempBroken = (t < 20 || t > 4075);  // open or shorted thermistor
  if (!tempBroken) {
    const float BETA = 3950;
    tempRaw = 1 / (log(1 / (4095. / t - 1)) / BETA + 1.0 / 298.15) - 273.15;
    if (digitalRead(PIN_NOISE) == LOW) tempRaw += noise2C();
  }

  // 4-20 mA: volts across the shunt -> milliamps -> bar
  int p = analogRead(PIN_PRESSURE);
  float volts = p * 3.3 / 4095.0;
  mA = volts / SHUNT_OHM * 1000.0;
  loopBroken = (mA < MA_MIN || mA > MA_MAX);
  bar = (mA - 4.0) * BAR_FULL / 16.0;
  if (bar < 0) bar = 0;

  // homework: low air = below 4.0 bar for 2 s, cleared only above 4.5 bar
  if (!loopBroken && bar < AIR_LOW_BAR) {
    if (airLowSince == 0) airLowSince = millis();
    if (!lowAir && millis() - airLowSince >= AIR_LOW_MS) {
      lowAir = true;
      Serial.println("LOW AIR: below 4.0 bar for 2 s");
    }
  } else {
    airLowSince = 0;
  }
  if (lowAir && !loopBroken && bar > AIR_OK_BAR) {
    lowAir = false;
    Serial.println("air OK: above 4.5 bar");
  }

  // decide
  bool wasHot = hot;
  if (smartMode()) {
    tempC += FILTER_ALPHA * (tempRaw - tempC);         // low-pass filter
    if (!hot && tempC >= TEMP_WARN_ON) hot = true;     // hysteresis:
    if (hot && tempC < TEMP_WARN_OFF) hot = false;     // two thresholds
  } else {
    tempC = tempRaw;                                    // believe every sample
    hot = tempC >= TEMP_WARN_ON;                        // one threshold
  }
  if (hot && !wasHot) {
    alarmCount++;
    chirpUntil = millis() + 60;
    Serial.print("HOT alarm #");
    Serial.println(alarmCount);
  }
}

void updateStateMachine() {
  bool start = pressed(btnStart);
  bool stop  = pressed(btnStop);
  bool reset = pressed(btnReset);
  bool jam   = pressed(btnJam);
  bool material = materialPresent();

  // protections first: a broken sensor or a hot motor beats any button
  if (state == RUNNING || state == IDLE) {
    if (sensorFault())          { setState(FAULT, SENSOR_FAULT);   return; }
    if (tempC >= TEMP_TRIP)     { setState(FAULT, MOTOR_OVERLOAD); return; }
  }

  switch (state) {
    case STOPPED:
      if (start) {
        if (sensorFault())   Serial.println("start refused: sensor fault");
        else if (tempC >= TEMP_RESET_OK) Serial.println("start refused: motor too hot");
        else if (!material) setState(IDLE, MATERIAL_SHORTAGE);
        else if (lowAir)    setState(IDLE, LOW_AIR);
        else                setState(RUNNING, NONE);
      }
      break;
    case RUNNING:
      if (stop)           setState(STOPPED, NONE);
      else if (jam)       setState(FAULT, JAM);
      else if (!material) setState(IDLE, MATERIAL_SHORTAGE);
      else if (lowAir)    setState(IDLE, LOW_AIR);
      break;
    case IDLE:
      if (stop)           setState(STOPPED, NONE);
      else if (!material) setState(IDLE, MATERIAL_SHORTAGE);
      else if (lowAir)    setState(IDLE, LOW_AIR);
      else                setState(RUNNING, NONE);
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

// ---------- count parts (Episode 2) ----------
void updateCounter() {
  if (micros() - lastEdgeUs < QUIET_US) return;
  bool level = digitalRead(PIN_PART);
  if (level == partStable) return;
  partStable = level;
  if (level == LOW && state == RUNNING) {
    unsigned long now = millis();
    if (lastPartMs != 0) cycleDs = (now - lastPartMs + 50) / 100;
    lastPartMs = now;
    goodCount++;
  }
}

// ---------- outputs ----------
void updateAndon() {
  unsigned long now = millis();
  bool blink = (now / 400) % 2;
  bool fast  = (now / 150) % 2;
  digitalWrite(PIN_GREEN,  state == RUNNING);
  digitalWrite(PIN_YELLOW, state == IDLE || (hot && state != FAULT && fast));
  digitalWrite(PIN_RED,    state == FAULT && blink);

  // buzzer: fault beeps, a new HOT alarm chirps once
  static int buzzing = 0;
  int want = 0;
  if (state == FAULT && blink) want = 880;
  else if (now < chirpUntil)   want = 1500;
  if (want != buzzing) {
    buzzing = want;
    if (buzzing) tone(PIN_BUZZER, buzzing);
    else noTone(PIN_BUZZER);
  }
}

void print1(float v) {               // one decimal, works on LCD and Serial
  int x = (int)(v * 10 + (v < 0 ? -0.5 : 0.5));
  if (x < 0) { lcd.print("-"); x = -x; }
  lcd.print(x / 10);
  lcd.print(".");
  lcd.print(x % 10);
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

  // LCD line 1: state + alarm count, line 2: temperature + pressure
  lcd.setCursor(0, 0);
  lcd.print(STATE_NAMES[state]);
  if (state == FAULT || state == IDLE) lcd.print(REASON_SHORT[reason]);
  else lcd.print(hot ? " HOT" : "    ");
  lcd.print(" ");
  lcd.print("A");
  lcd.print(alarmCount);
  lcd.print("   ");
  lcd.setCursor(0, 1);
  if (tempBroken) lcd.print("T ---- ");
  else { lcd.print("T"); print1(tempC); lcd.print("C "); }
  if (loopBroken) lcd.print("P WIRE!  ");
  else { lcd.print("P"); print1(bar); lcd.print("bar   "); }

  Serial.print("{\"state\":");
  Serial.print(state);
  Serial.print(",\"reason\":");
  Serial.print(reason);
  Serial.print(",\"temp_c\":");
  serial1(tempC);
  Serial.print(",\"hot\":");
  Serial.print(hot ? 1 : 0);
  Serial.print(",\"alarms\":");
  Serial.print(alarmCount);
  Serial.print(",\"ma\":");
  serial1(mA);
  Serial.print(",\"bar\":");
  serial1(bar);
  Serial.print(",\"low_air\":");
  Serial.print(lowAir ? 1 : 0);
  Serial.print(",\"good\":");
  Serial.print(goodCount);
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
  pinMode(PIN_NOISE, INPUT_PULLUP);
  pinMode(PIN_SMART, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_PART), onPartChange, CHANGE);

  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_YELLOW, OUTPUT);
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  analogReadResolution(12);
  lcd.init();
  lcd.backlight();

  // start the filter at the real value, not at 25 C
  lastSample = millis() - 1000;
  readSensors();
  tempC = tempRaw;
  hot = false;
  alarmCount = 0;

  Serial.println("ChinarSoft machine ready. Drag the NTC and the pressure knob, press START.");
  stateSince = millis();
}

void loop() {
  readSensors();
  updateStateMachine();
  updateCounter();
  updateAndon();
  report();
  delay(5);
}
