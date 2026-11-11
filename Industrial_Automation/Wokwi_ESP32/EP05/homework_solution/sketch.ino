// ChinarSoft — Industrial ESP32 with Wokwi, Episode 5 — HOMEWORK SOLUTION
// Function 16 (write multiple registers): HR11 = part target, HR12 = batch number, in one frame.
//   request: id, 16, start hi/lo, quantity hi/lo, byte count, values..., CRC
//   reply:   id, 16, start hi/lo, quantity hi/lo, CRC   (8 bytes - NOT an echo of the request)
//   exception 2 for any address outside HR11..HR12, exception 3 for a bad quantity / byte count.
// New Serial Monitor commands: t = write target 500 + batch 1234 (FC16),
//   b = FC16 to HR20 (exception 2), r = read HR11..HR12 back (FC3).
// See a Modbus RTU frame: one ESP32, two UARTs.
//
// Same machine as Episode 4 (andon, buttons, part counter, NTC, 4-20 mA, K1 motor + K2 clamp
// relays, guard door) - and now it speaks Modbus RTU.
//
// One ESP32 plays two devices on one wire:
//   UART2 = the MACHINE (Modbus slave, id 1)  RX GPIO 36, TX GPIO 5
//   UART1 = "SCADA"     (Modbus master)       RX GPIO 39, TX GPIO 15
//   Wires: GPIO 15 -> GPIO 36 (requests), GPIO 5 -> GPIO 39 (replies). 9600 baud, 8N1.
//   A logic analyzer watches both wires (D0 = request, D1 = reply).
//
// SCADA reads holding registers 0-6 every second (function 3):
//   HR0 state  HR1 reason  HR2 good parts  HR3 cycle time (0.1 s)
//   HR4 motor temp (0.1 C)  HR5 air pressure (0.1 bar)  HR6 bits: 1 motor, 2 clamp, 4 door closed
// and writes HR10 (function 6): 1 = START, 2 = STOP, 3 = RESET.
// A remote command goes through the same checks as the button: Modbus says "got it",
// the machine still decides.
//
// Type into the Serial Monitor:  1 / 2 / 3 = remote START / STOP / RESET
//                                c = corrupt the next request (CRC demo)
//                                e = read a register that does not exist (exception demo)
//
// On real hardware the slave is a separate device and the wire is RS-485 (MAX485 + DE pin).
// GPIO 5 and 15 are strapping pins, but a UART TX idles HIGH - the same level they want at boot.
//
// State codes = ChinarSoft Virtual Machine HR0: 0 STOPPED, 1 RUNNING, 2 IDLE, 3 FAULT
// Reasons     = HR1: 0 none, 1 material shortage, 2 jam, 3 motor overload, 4 sensor fault,
//               9 guard open (our own code)

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
const int PIN_K2_CLAMP = 17;  // (a relay must not sit on strapping pins 0, 2, 5, 12, 15)

// Modbus RTU wires (Episode 5)
const int PIN_SCADA_TX = 15;  // -> GPIO 36
const int PIN_SCADA_RX = 39;  // <- GPIO 5   (36 and 39 are input-only: perfect for RX)
const int PIN_MACH_TX  = 5;
const int PIN_MACH_RX  = 36;

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

// Episode 5: bytes collected from one UART until the line goes quiet
// (structs live up here: the Arduino builder puts function prototypes above the first function)
struct FrameRx {
  uint8_t buf[64];
  int len;
  unsigned long lastUs;
};

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
int remoteCmd = 0;                // Episode 5: set by a Modbus write, used once by the state machine

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
  bool start = pressed(btnStart) || remoteCmd == 1;   // a remote command = the same button
  bool stop  = pressed(btnStop)  || remoteCmd == 2;
  bool reset = pressed(btnReset) || remoteCmd == 3;
  remoteCmd = 0;
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


// ---------- Modbus RTU (Episode 5) ----------
const uint32_t MB_BAUD = 9600;
const uint8_t  SLAVE_ID = 1;
const unsigned long FRAME_GAP_US = 4000;  // a frame ends after 3.5 silent characters (3.65 ms)
const unsigned long POLL_MS = 1000;
const unsigned long TIMEOUT_MS = 200;
const int HR_COUNT = 7;                   // HR0..HR6
const int HR_COMMAND = 10;

HardwareSerial& scada   = Serial1;
HardwareSerial& machine = Serial2;

uint32_t mbOk = 0;            // frames the machine accepted
uint32_t mbCrcErr = 0;        // frames the machine threw away
uint16_t partTarget = 0;      // homework: HR11, written with function 16
uint16_t batchNo = 0;         // homework: HR12
const char* CMD_NAMES[] = { "", "START", "STOP", "RESET" };

// CRC-16/MODBUS: start 0xFFFF, polynomial 0xA001 (reflected 0x8005), sent low byte first
uint16_t crc16(const uint8_t* b, int n) {
  uint16_t crc = 0xFFFF;
  for (int i = 0; i < n; i++) {
    crc ^= b[i];
    for (int k = 0; k < 8; k++) crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
  }
  return crc;
}

void addCrc(uint8_t* b, int n) {
  uint16_t c = crc16(b, n);
  b[n] = c & 0xFF;
  b[n + 1] = c >> 8;
}

bool crcOk(const uint8_t* b, int n) {
  if (n < 4) return false;
  uint16_t c = crc16(b, n - 2);
  return b[n - 2] == (c & 0xFF) && b[n - 1] == (c >> 8);
}

void printFrame(const char* who, const uint8_t* b, int n) {
  Serial.print(who);
  for (int i = 0; i < n; i++) {
    if (b[i] < 16) Serial.print("0");
    Serial.print(b[i], HEX);
    Serial.print(i == n - 1 ? "" : " ");
  }
  Serial.println();
}

// Modbus RTU has no start or end byte: a frame ends when the line goes quiet (FrameRx above).
FrameRx slaveRx = { {0}, 0, 0 };
FrameRx scadaRx = { {0}, 0, 0 };

int receiveFrame(HardwareSerial& port, FrameRx& f) {
  while (port.available()) {
    uint8_t c = port.read();
    if (f.len < (int)sizeof(f.buf)) f.buf[f.len++] = c;
    f.lastUs = micros();
  }
  if (f.len > 0 && micros() - f.lastUs >= FRAME_GAP_US) {
    int n = f.len;
    f.len = 0;
    return n;
  }
  return 0;
}

// ---- the machine side (slave) ----
bool readable(int a) { return (a >= 0 && a < HR_COUNT) || a == HR_COMMAND || a == 11 || a == 12; }

uint16_t holding(int a) {
  switch (a) {
    case 0: return state;
    case 1: return reason;
    case 2: return goodCount & 0xFFFF;
    case 3: return cycleDs;
    case 4: return (uint16_t)(int16_t)lround(tempC * 10);
    case 5: return (uint16_t)lround(bar * 10);
    case 6: return (k1Motor ? 1 : 0) | (k2Clamp ? 2 : 0) | (doorClosed() ? 4 : 0);
    case 11: return partTarget;
    case 12: return batchNo;
    default: return 0;   // HR10 reads back 0: a command is an event, not a value
  }
}

void slaveSend(uint8_t* r, int n) {
  addCrc(r, n);
  machine.write(r, n + 2);
}

void slaveException(uint8_t fc, uint8_t code) {
  uint8_t r[5] = { SLAVE_ID, (uint8_t)(fc | 0x80), code };
  slaveSend(r, 3);
}

void modbusSlave() {
  int n = receiveFrame(machine, slaveRx);
  if (n == 0) return;
  uint8_t* f = slaveRx.buf;
  if (!crcOk(f, n)) {                 // a broken frame gets NO reply - not even an error
    mbCrcErr++;
    Serial.println("  machine: CRC wrong -> frame ignored, no reply");
    return;
  }
  if (f[0] != SLAVE_ID) return;       // for another device on the bus
  mbOk++;
  uint8_t fc = f[1];
  if (fc == 16) {                                    // homework: write multiple registers
    if (n < 11) { slaveException(fc, 3); return; }
    uint16_t start = (f[2] << 8) | f[3];
    uint16_t qty   = (f[4] << 8) | f[5];
    uint8_t  bytes = f[6];
    if (qty < 1 || qty > 2 || bytes != qty * 2 || n != 9 + bytes) { slaveException(fc, 3); return; }
    if (start < 11 || start + qty - 1 > 12)                       { slaveException(fc, 2); return; }
    for (int i = 0; i < qty; i++) {
      uint16_t v = (f[7 + i * 2] << 8) | f[8 + i * 2];
      if (start + i == 11) partTarget = v; else batchNo = v;
    }
    uint8_t r[8] = { SLAVE_ID, 16, f[2], f[3], f[4], f[5] };   // start + quantity, then CRC
    slaveSend(r, 6);
    return;
  }
  if (n != 8) { slaveException(fc, 3); return; }
  uint16_t addr = (f[2] << 8) | f[3];
  uint16_t val  = (f[4] << 8) | f[5];

  if (fc == 3) {                                     // read holding registers
    if (val < 1 || val > 20) { slaveException(fc, 3); return; }
    for (int a = addr; a < addr + val; a++)
      if (!readable(a)) { slaveException(fc, 2); return; }
    uint8_t r[48] = { SLAVE_ID, 3, (uint8_t)(val * 2) };
    for (int i = 0; i < val; i++) {
      uint16_t v = holding(addr + i);
      r[3 + i * 2] = v >> 8;                         // Modbus registers are big-endian
      r[4 + i * 2] = v & 0xFF;
    }
    slaveSend(r, 3 + val * 2);
  } else if (fc == 6) {                              // write single register
    if (addr != HR_COMMAND)  { slaveException(fc, 2); return; }
    if (val < 1 || val > 3)  { slaveException(fc, 3); return; }
    remoteCmd = val;
    machine.write(f, 8);                             // the reply is an exact echo
  } else {
    slaveException(fc, 1);                           // illegal function
  }
}

// ---- the SCADA side (master) ----
bool waiting = false;
unsigned long sentAt = 0;
unsigned long lastPoll = 0;
int pendingWrite = 0;
bool corruptNext = false;
bool badAddressNext = false;
int pending16 = 0;            // homework: 1 = write HR11..12, 2 = write HR20 (bad), 3 = read HR11..12

void masterSend16(uint16_t start, uint16_t a, uint16_t b) {
  uint8_t q[13] = { SLAVE_ID, 16, (uint8_t)(start >> 8), (uint8_t)(start & 0xFF), 0, 2, 4,
                    (uint8_t)(a >> 8), (uint8_t)(a & 0xFF), (uint8_t)(b >> 8), (uint8_t)(b & 0xFF) };
  addCrc(q, 11);
  printFrame("SCADA   -> ", q, 13);
  scada.write(q, 13);
  waiting = true;
  sentAt = millis();
}

void masterSend(uint8_t fc, uint16_t addr, uint16_t val) {
  uint8_t q[8] = { SLAVE_ID, fc, (uint8_t)(addr >> 8), (uint8_t)(addr & 0xFF),
                   (uint8_t)(val >> 8), (uint8_t)(val & 0xFF) };
  addCrc(q, 6);
  if (corruptNext) {                  // noise on the wire: one bit flips AFTER the CRC was made
    corruptNext = false;
    q[5] ^= 0x01;
    Serial.println("SCADA: one bit flipped on the wire (byte 6)");
  }
  printFrame("SCADA   -> ", q, 8);
  scada.write(q, 8);
  waiting = true;
  sentAt = millis();
}

const char* EXC_NAMES[] = { "", "illegal function", "illegal data address", "illegal data value" };

void handleReply(const uint8_t* f, int n) {
  printFrame("MACHINE <- ", f, n);
  if (!crcOk(f, n)) { Serial.println("  SCADA: reply CRC wrong"); return; }
  if (f[1] & 0x80) {
    Serial.print("  exception ");
    Serial.print(f[2]);
    Serial.print(": ");
    Serial.println(f[2] <= 3 ? EXC_NAMES[f[2]] : "?");
    return;
  }
  if (f[1] == 6) {
    Serial.println("  write confirmed (echo)");
    return;
  }
  if (f[1] == 16) {
    Serial.print("  FC16 confirmed: ");
    Serial.print((f[4] << 8) | f[5]);
    Serial.print(" registers from HR");
    Serial.println((f[2] << 8) | f[3]);
    return;
  }
  if (f[1] == 3 && f[2] == 4) {
    Serial.print("  HR11 target ");
    Serial.print((f[3] << 8) | f[4]);
    Serial.print(" | HR12 batch ");
    Serial.println((f[5] << 8) | f[6]);
    return;
  }
  if (f[1] == 3 && f[2] == HR_COUNT * 2) {
    uint16_t r[HR_COUNT];
    for (int i = 0; i < HR_COUNT; i++) r[i] = (f[3 + i * 2] << 8) | f[4 + i * 2];
    Serial.print("  ");
    Serial.print(r[0] <= 3 ? STATE_NAMES[r[0]] : "?");
    Serial.print(" reason ");
    Serial.print(r[1]);
    Serial.print(" | good ");
    Serial.print(r[2]);
    Serial.print(" | ");
    serial1((int16_t)r[4] / 10.0);
    Serial.print(" C | ");
    serial1(r[5] / 10.0);
    Serial.print(" bar | motor ");
    Serial.print(r[6] & 1 ? "ON" : "off");
    Serial.print(" door ");
    Serial.println(r[6] & 4 ? "closed" : "OPEN");
  }
}

void modbusMaster() {
  while (Serial.available()) {        // commands typed into the Serial Monitor
    char c = Serial.read();
    if (c >= '1' && c <= '3') pendingWrite = c - '0';
    else if (c == 'c' || c == 'C') corruptNext = true;
    else if (c == 'e' || c == 'E') badAddressNext = true;
    else if (c == 't' || c == 'T') pending16 = 1;
    else if (c == 'b' || c == 'B') pending16 = 2;
    else if (c == 'r' || c == 'R') pending16 = 3;
  }

  if (waiting) {
    int n = receiveFrame(scada, scadaRx);
    if (n > 0) {
      waiting = false;
      handleReply(scadaRx.buf, n);
    } else if (millis() - sentAt > TIMEOUT_MS) {
      waiting = false;
      Serial.println("  SCADA: timeout - no reply in 200 ms");
    }
    return;                           // one question at a time on the bus
  }

  if (pending16) {                    // homework commands
    int p = pending16;
    pending16 = 0;
    if (p == 1) { Serial.println("SCADA: FC16 write HR11 = 500 (target), HR12 = 1234 (batch)"); masterSend16(11, 500, 1234); }
    if (p == 2) { Serial.println("SCADA: FC16 write HR20..21 (not supported)");               masterSend16(20, 1, 2); }
    if (p == 3) { Serial.println("SCADA: read HR11..12");                                      masterSend(3, 11, 2); }
    return;
  }

  if (pendingWrite) {
    Serial.print("SCADA: write HR10 = ");
    Serial.print(pendingWrite);
    Serial.print(" (");
    Serial.print(CMD_NAMES[pendingWrite]);
    Serial.println(")");
    masterSend(6, HR_COMMAND, pendingWrite);
    pendingWrite = 0;
    return;
  }

  if (millis() - lastPoll >= POLL_MS) {
    lastPoll = millis();
    if (badAddressNext) {
      badAddressNext = false;
      Serial.println("SCADA: read HR50 (does not exist)");
      masterSend(3, 50, 1);
    } else {
      masterSend(3, 0, HR_COUNT);
    }
  }
}

unsigned long lastReport = 0;

void report() {
  if (millis() - lastReport < 500) return;
  lastReport = millis();

  // LCD line 1: state + reason (+ DOOR when open), line 2: Modbus frames ok / thrown away
  lcd.setCursor(0, 0);
  lcd.print(STATE_NAMES[state]);
  lcd.print(REASON_SHORT[reason]);
  if (!doorClosed() && reason != GUARD_OPEN) lcd.print(" DOOR");
  lcd.print("        ");
  lcd.setCursor(0, 1);
  lcd.print("MB ok:");
  lcd.print(mbOk);
  lcd.print(" crc:");
  lcd.print(mbCrcErr);
  lcd.print("    ");
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

  scada.begin(MB_BAUD, SERIAL_8N1, PIN_SCADA_RX, PIN_SCADA_TX);
  machine.begin(MB_BAUD, SERIAL_8N1, PIN_MACH_RX, PIN_MACH_TX);

  analogReadResolution(12);
  lcd.init();
  lcd.backlight();

  // start the temperature filter at the real value
  int t = analogRead(PIN_TEMP);
  if (t > 20 && t < 4075) tempC = 1 / (log(1 / (4095. / t - 1)) / 3950 + 1.0 / 298.15) - 273.15;

  Serial.println("ChinarSoft machine ready. Modbus RTU id 1, 9600 8N1.");
  Serial.println("Type 1/2/3 = remote START/STOP/RESET, c = corrupt next frame, e = bad address.");
  Serial.println("Homework: t = FC16 write target + batch, b = FC16 to a bad address, r = read them back.");
  stateSince = millis();
}

void loop() {
  readSensors();
  updateStateMachine();
  updateRelays();
  updateCounter();
  updateAndon();
  modbusMaster();
  modbusSlave();
  report();
  delay(5);
}
