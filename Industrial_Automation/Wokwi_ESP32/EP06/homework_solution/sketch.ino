// ChinarSoft — Industrial ESP32 with Wokwi, Episode 6 — HOMEWORK SOLUTION
// Retained alarm topic: chinarsoft/demo/<id>/alarm
//   FAULT  -> publish the reason ("jam", "motor overtemp", ...) RETAINED
//   leaving FAULT (RESET) -> publish an EMPTY retained message = the broker deletes it
//   A dashboard that connects later sees the active alarm at once - or nothing, if it is cleared.
//   After a reconnect the alarm is sent again, so the broker always matches the machine.
// Publish machine data with MQTT: JSON, topic design, retained messages and a last will.
//
// Same machine as Episode 4 (andon, START / STOP / RESET / JAM, material, PART counter, NTC,
// 4-20 mA air pressure, K1 MOTOR + K2 CLAMP relays, guard door) - now on the network.
//
// Two cores, two jobs:
//   core 1 (loop)        = the machine. It never waits for the network.
//   core 0 (networkTask) = Wi-Fi + MQTT. It reads a snapshot of the machine and a queue of events.
//
// Topics (every run gets its own random id, printed at boot):
//   chinarsoft/demo/<id>/status  "online" / "offline"   retained, "offline" is the LAST WILL
//   chinarsoft/demo/<id>/state   JSON snapshot           retained, on every change + every 5 s
//   chinarsoft/demo/<id>/event   text events             not retained
//   chinarsoft/demo/<id>/cmd     subscribe: start / stop / reset (same checks as the buttons)
//
// Watch it: any MQTT client -> broker.hivemq.com:1883, subscribe to chinarsoft/demo/<id>/#
// LINK switch (GPIO 12): flip it to cut the "cable" silently - the broker notices after
// 1.5 x keep-alive (15 s) and publishes the last will for us.
//
// State codes = ChinarSoft Virtual Machine HR0: 0 STOPPED, 1 RUNNING, 2 IDLE, 3 FAULT
// Reasons     = HR1: 0 none, 1 material shortage, 2 jam, 3 motor overload, 4 sensor fault,
//               9 guard open (our own code)

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>
#include <WiFi.h>
#include <PubSubClient.h>

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
const int PIN_LINK     = 12;  // Episode 6: "network cable" switch, connected to GND = link OK

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
volatile int remoteCmd = 0;       // Episode 6: set by an MQTT command (core 0), used once by core 1

// ---------- shared between the cores (Episode 6) ----------
struct Snapshot {
  uint8_t state, reason, motor, clamp, door;
  float tempC, bar;
  uint32_t good;
  uint16_t cycleDs;
};
Snapshot snap;                                         // written by core 1, read by core 0
portMUX_TYPE snapMux = portMUX_INITIALIZER_UNLOCKED;   // a tiny lock around the copy
QueueHandle_t eventQ;                                  // events waiting for the network
volatile uint32_t eventsDropped = 0;                   // queue full -> counted, not blocked

// The machine never waits for the network: if the queue is full, the event is dropped.
void logEvent(const char* text) {
  Serial.println(text);
  char item[64];
  strncpy(item, text, sizeof(item) - 1);
  item[sizeof(item) - 1] = 0;
  if (xQueueSend(eventQ, item, 0) != pdTRUE) eventsDropped++;
}

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
  char ev[64];
  if (reason != NONE) snprintf(ev, sizeof(ev), "-> %s (%s)", STATE_NAMES[state], REASON_NAMES[reason]);
  else snprintf(ev, sizeof(ev), "-> %s", STATE_NAMES[state]);
  logEvent(ev);
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
  int cmd = remoteCmd;               // a remote command = the same button
  remoteCmd = 0;
  bool start = pressed(btnStart) || cmd == 1;
  bool stop  = pressed(btnStop)  || cmd == 2;
  bool reset = pressed(btnReset) || cmd == 3;
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
        if (!doorClosed())               logEvent("start refused: guard door open");
        else if (sensorFault())          logEvent("start refused: sensor fault");
        else if (tempC >= TEMP_RESET_OK) logEvent("start refused: motor too hot");
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
        if (sensorFault())               logEvent("reset refused: sensor fault");
        else if (tempC >= TEMP_RESET_OK) logEvent("reset refused: motor too hot");
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

// ---------- network (Episode 6): everything below runs on core 0 ----------
const char* WIFI_SSID = "Wokwi-GUEST";
const char* MQTT_HOST = "broker.hivemq.com";
const uint16_t MQTT_PORT = 1883;
const uint16_t KEEPALIVE_S = 10;          // the broker gives up after 1.5 x 10 s of silence
const unsigned long STATE_EVERY_MS = 5000;

WiFiClient net;
PubSubClient mqtt(net);
char clientId[24];
char tStatus[48], tState[48], tEvent[48], tCmd[48], tAlarm[48];
int alarmSent = -1;          // homework: reason last published on .../alarm (0 = cleared, -1 = unknown)
volatile bool wifiOk = false, mqttOk = false, linkOk = true;
volatile uint32_t published = 0;

void onCommand(char* topic, byte* payload, unsigned int len) {
  char c[16];
  unsigned int n = len < sizeof(c) - 1 ? len : sizeof(c) - 1;
  for (unsigned int i = 0; i < n; i++) c[i] = tolower(payload[i]);
  c[n] = 0;
  Serial.print("MQTT <- cmd: ");
  Serial.println(c);
  if      (!strcmp(c, "start")) remoteCmd = 1;
  else if (!strcmp(c, "stop"))  remoteCmd = 2;
  else if (!strcmp(c, "reset")) remoteCmd = 3;
  else logEvent("unknown command");
}

int stateJson(char* out, size_t size, const Snapshot& s) {
  int tx = (int)lround(s.tempC * 10), bx = (int)lround(s.bar * 10);
  return snprintf(out, size,
    "{\"state\":%u,\"reason\":%u,\"motor\":%u,\"clamp\":%u,\"door\":%u,"
    "\"temp_c\":%d.%d,\"bar\":%d.%d,\"good\":%lu,\"cycle_s\":%u.%u,\"dropped\":%lu}",
    s.state, s.reason, s.motor, s.clamp, s.door, tx / 10, abs(tx % 10), bx / 10, bx % 10,
    (unsigned long)s.good, s.cycleDs / 10, s.cycleDs % 10, (unsigned long)eventsDropped);
}

unsigned long lastTry = 0;
bool neverTried = true;
unsigned long lastState = 0;
Snapshot lastSent;
bool sendState = true;

void networkStep() {
  linkOk = digitalRead(PIN_LINK) == LOW;
  if (!linkOk) return;                    // cable cut: we go silent, nothing is sent or read

  if (WiFi.status() != WL_CONNECTED) {
    wifiOk = mqttOk = false;
    if (neverTried || millis() - lastTry > 5000) {
      neverTried = false;
      lastTry = millis();
      Serial.println("WiFi: connecting ...");
      WiFi.begin(WIFI_SSID, "", 6);
    }
    return;
  }
  wifiOk = true;

  if (!mqtt.connected()) {
    if (mqttOk) Serial.println("MQTT: connection lost");
    mqttOk = false;
    if (millis() - lastTry > 2000) {
      lastTry = millis();
      Serial.println("MQTT: connecting ...");
      // the last will: the BROKER publishes "offline" (retained) if we vanish without saying goodbye
      if (mqtt.connect(clientId, nullptr, nullptr, tStatus, 1, true, "offline")) {
        mqtt.publish(tStatus, "online", true);
        mqtt.subscribe(tCmd, 1);
        Serial.println("MQTT: connected - status online (retained), last will = offline");
        mqttOk = true;
        sendState = true;
        alarmSent = -1;                   // homework: resend the alarm after every reconnect
      } else {
        Serial.print("MQTT: failed, rc=");
        Serial.println(mqtt.state());
      }
    }
    return;
  }
  mqttOk = true;
  mqtt.loop();                            // keep-alive pings + incoming commands

  char ev[64];
  while (xQueueReceive(eventQ, ev, 0) == pdTRUE) {
    mqtt.publish(tEvent, ev, false);      // events: not retained - they are history, not state
    published++;
  }

  Snapshot s;
  portENTER_CRITICAL(&snapMux);
  s = snap;
  portEXIT_CRITICAL(&snapMux);
  if (s.state != lastSent.state || s.reason != lastSent.reason || s.motor != lastSent.motor ||
      s.clamp != lastSent.clamp || s.door != lastSent.door || s.good != lastSent.good) sendState = true;
  // homework: retained alarm = the reason while in FAULT, an empty retained message clears it
  int alarmNow = (s.state == FAULT) ? s.reason : 0;
  if (alarmNow != alarmSent) {
    mqtt.publish(tAlarm, alarmNow ? REASON_NAMES[alarmNow] : "", true);
    Serial.print("MQTT -> alarm = ");
    Serial.println(alarmNow ? REASON_NAMES[alarmNow] : "(empty, retained deleted)");
    alarmSent = alarmNow;
  }

  if (sendState || millis() - lastState >= STATE_EVERY_MS) {
    char js[200];
    stateJson(js, sizeof(js), s);
    mqtt.publish(tState, js, true);       // state: retained - a new dashboard sees it at once
    published++;
    lastSent = s;
    lastState = millis();
    sendState = false;
  }
}

void networkTask(void*) {
  for (;;) {
    networkStep();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ---------- core 1: copy the machine into the snapshot, LCD ----------

void report() {
  portENTER_CRITICAL(&snapMux);
  snap.state = state;
  snap.reason = reason;
  snap.motor = k1Motor;
  snap.clamp = k2Clamp;
  snap.door = doorClosed();
  snap.tempC = tempC;
  snap.bar = bar;
  snap.good = goodCount;
  snap.cycleDs = cycleDs;
  portEXIT_CRITICAL(&snapMux);

  if (millis() - lastReport < 500) return;
  lastReport = millis();

  // LCD line 1: state + reason, line 2: network
  lcd.setCursor(0, 0);
  lcd.print(STATE_NAMES[state]);
  lcd.print(REASON_SHORT[reason]);
  if (!doorClosed() && reason != GUARD_OPEN) lcd.print(" DOOR");
  lcd.print("        ");
  lcd.setCursor(0, 1);
  if (!linkOk)      lcd.print("LINK CUT q:");
  else if (!wifiOk) lcd.print("WiFi ...   q:");
  else if (!mqttOk) lcd.print("MQTT ...   q:");
  else              lcd.print("MQTT ok    q:");
  lcd.print((int)uxQueueMessagesWaiting(eventQ));
  lcd.print("   ");
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
  pinMode(PIN_LINK, INPUT_PULLUP);
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

  // Episode 6: topics with a random id per run, the event queue, the network task on core 0
  eventQ = xQueueCreate(16, 64);
  uint32_t id = esp_random() & 0xFFFFFF;
  snprintf(clientId, sizeof(clientId), "chinarsoft-%06lx", (unsigned long)id);
  snprintf(tStatus, sizeof(tStatus), "chinarsoft/demo/%06lx/status", (unsigned long)id);
  snprintf(tState,  sizeof(tState),  "chinarsoft/demo/%06lx/state",  (unsigned long)id);
  snprintf(tEvent,  sizeof(tEvent),  "chinarsoft/demo/%06lx/event",  (unsigned long)id);
  snprintf(tCmd,    sizeof(tCmd),    "chinarsoft/demo/%06lx/cmd",    (unsigned long)id);
  snprintf(tAlarm,  sizeof(tAlarm),  "chinarsoft/demo/%06lx/alarm",  (unsigned long)id);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setKeepAlive(KEEPALIVE_S);
  mqtt.setBufferSize(512);
  mqtt.setCallback(onCommand);
  WiFi.mode(WIFI_STA);
  xTaskCreatePinnedToCore(networkTask, "net", 8192, nullptr, 1, nullptr, 0);

  Serial.println("ChinarSoft machine ready. Close the guard door (D), then START.");
  Serial.print("MQTT: broker.hivemq.com:1883 - subscribe to chinarsoft/demo/");
  Serial.print(clientId + 11);
  Serial.println("/#");
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
