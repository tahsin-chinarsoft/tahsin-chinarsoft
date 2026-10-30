// ChinarSoft - Arduino Sensor Kit, Episode 10 - TASK SOLUTION
// Task: make the joystick's X axis change the value in steps of ten (big changes are quick).
//
// What changed vs E10_MenuController:
//   - New block 2b in loop(): while EDITING, pushing the joystick left/right (X axis)
//     calls move(+/-10), i.e. ten setting steps per auto-repeat
//     (Speed: 10, Temp: 50, Count: 10, Delay ms: 500). Values are still clamped to min/max.
//   - X axis has its own repeat timer (lastJoyX), same JOY_REPEAT_MS and DEADZONE as Y.
//   - Y axis (one step), encoder, push buttons and touch-to-save/EEPROM layout unchanged.
//   - Outside edit mode the X axis does nothing.
//
// Wiring: same wiring as E10_MenuController, no extra parts (listed below).
//
// Base project: Joystick (KY-023), Rotary encoder (KY-040), Metal touch sensor (KY-036)
// Machine settings menu (Serial Monitor)
//   Joystick left-right (editing)   : TASK: change the value 10 steps at a time
//   Encoder turn / joystick up-down : move through the menu, or change a value while editing
//   Encoder push / joystick push    : enter / leave edit mode
//   Touch the KY-036 wire           : SAVE all settings to EEPROM (they survive a reset / power cut)
//
// Wiring (Arduino Uno):
//   KY-040 encoder   CLK -> D2 | DT -> D3 | SW -> D5 | + -> 5V | GND -> GND
//   KY-023 joystick  VRx -> A0 | VRy -> A1 | SW -> D4 | +5V -> 5V | GND -> GND
//   KY-036 touch     AO -> A2  | DO -> D6 | + -> 5V | G -> GND

#include <EEPROM.h>

const int ENC_CLK = 2, ENC_DT = 3, ENC_SW = 5;
const int JOY_X = A0, JOY_Y = A1, JOY_SW = 4;
const int TOUCH_DO = 6;

const int DEADZONE = 150;               // joystick center +/- this = no action
const unsigned long JOY_REPEAT_MS = 250;

struct Setting { const char *name; int value; int minV; int maxV; int step; };
Setting menu[] = {
  {"Speed",     50,  0,  100,  1},
  {"Temp",     180, 20,  250,  5},
  {"Count",     12,  1,   99,  1},
  {"Delay ms", 500,  0, 5000, 50},
};
const int MENU_LEN = sizeof(menu) / sizeof(menu[0]);

const int EEPROM_ADDR = 0;
const byte EEPROM_MAGIC = 0xA5;        // marks that valid settings are stored

int cursor = 0;
bool editing = false;
volatile int encSteps = 0;

// Quadrature decoding with a lookup table: rejects contact bounce
// Index = (old AB << 2) | new AB, value = -1, 0 or +1
const int8_t QUAD_TABLE[16] = {0,-1,1,0, 1,0,0,-1, -1,0,0,1, 0,1,-1,0};
volatile uint8_t encState = 0;
volatile int8_t encAccum = 0;

void onEncoder() {
  encState = ((encState << 2) | (digitalRead(ENC_CLK) << 1) | digitalRead(ENC_DT)) & 0x0F;
  encAccum += QUAD_TABLE[encState];
  if (encAccum >= 4)  { encSteps++; encAccum = 0; }   // KY-040: 4 transitions per click
  if (encAccum <= -4) { encSteps--; encAccum = 0; }
}

void saveSettings() {
  EEPROM.update(EEPROM_ADDR, EEPROM_MAGIC);
  for (int i = 0; i < MENU_LEN; i++) EEPROM.put(EEPROM_ADDR + 1 + i * sizeof(int), menu[i].value);
}

void loadSettings() {
  if (EEPROM.read(EEPROM_ADDR) != EEPROM_MAGIC) return;   // first run: keep defaults
  for (int i = 0; i < MENU_LEN; i++) {
    int v;
    EEPROM.get(EEPROM_ADDR + 1 + i * sizeof(int), v);
    menu[i].value = constrain(v, menu[i].minV, menu[i].maxV);
  }
}

void printMenu() {
  Serial.println();
  Serial.println("===== MACHINE SETTINGS =====");
  for (int i = 0; i < MENU_LEN; i++) {
    Serial.print(i == cursor ? (editing ? " * " : " > ") : "   ");
    Serial.print(menu[i].name);
    Serial.print(": ");
    Serial.println(menu[i].value);
  }
  Serial.println(editing ? "[editing - push to finish]" : "[turn to move, push to edit, touch to save]");
}

void move(int delta) {
  if (delta == 0) return;
  if (editing) {
    Setting &s = menu[cursor];
    s.value = constrain(s.value + delta * s.step, s.minV, s.maxV);
  } else {
    cursor = (cursor + delta + MENU_LEN) % MENU_LEN;
  }
  printMenu();
}

// Debounced falling-edge detector for active-LOW buttons
bool pressed(int pin, int &last, unsigned long &t) {
  int now = digitalRead(pin);
  bool edge = false;
  if (now != last && millis() - t > 40) {
    t = millis();
    edge = (now == LOW);
    last = now;
  }
  return edge;
}

void setup() {
  Serial.begin(115200);
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);
  pinMode(JOY_SW, INPUT_PULLUP);
  pinMode(TOUCH_DO, INPUT);
  attachInterrupt(digitalPinToInterrupt(ENC_CLK), onEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_DT), onEncoder, CHANGE);
  loadSettings();
  printMenu();
}

void loop() {
  // 1) Encoder
  noInterrupts();
  int steps = encSteps;
  encSteps = 0;
  interrupts();
  move(steps);

  // 2) Joystick (Y axis), with dead zone and auto-repeat
  static unsigned long lastJoy = 0;
  int y = analogRead(JOY_Y) - 512;
  if (abs(y) > DEADZONE && millis() - lastJoy > JOY_REPEAT_MS) {
    lastJoy = millis();
    move(y > 0 ? 1 : -1);               // flip the sign if your joystick is mounted upside down
  }

  // 2b) TASK: X axis = big steps while editing
  static unsigned long lastJoyX = 0;
  int x = analogRead(JOY_X) - 512;
  if (editing && abs(x) > DEADZONE && millis() - lastJoyX > JOY_REPEAT_MS) {
    lastJoyX = millis();
    move((x > 0 ? 1 : -1) * 10);        // TASK: ten steps. Flip the sign if right/left feel reversed
  }

  // 3) Push buttons toggle edit mode
  static int lastEnc = HIGH, lastJs = HIGH;
  static unsigned long tEnc = 0, tJs = 0;
  if (pressed(ENC_SW, lastEnc, tEnc) || pressed(JOY_SW, lastJs, tJs)) {
    editing = !editing;
    printMenu();
  }

  // 4) Touch = save. The KY-036 output buzzes at mains frequency while touched,
  //    so we accept one save and then ignore the sensor for 1.5 s.
  static unsigned long lastSave = 0;
  if (digitalRead(TOUCH_DO) == HIGH && millis() - lastSave > 1500) {
    lastSave = millis();
    saveSettings();                         // EEPROM.put uses update(): unchanged bytes are not rewritten
    Serial.println(">> Settings SAVED to EEPROM");
  }
}
