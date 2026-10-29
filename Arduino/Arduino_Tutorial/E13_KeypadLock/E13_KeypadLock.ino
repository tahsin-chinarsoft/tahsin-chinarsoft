// ChinarSoft - Arduino Sensor Kit, Episode 13
// 4x3 membrane keypad + relay (KY-019) + passive buzzer + two-color LED
// Project: Password door lock with EEPROM-saved PIN and brute-force lockout
//
// Keys:  digits = enter PIN   # = confirm   * = clear
//        To change the PIN: unlock first, then type  *0#  , then the new PIN (4-8 digits) and #.
//
// Wiring (Arduino Uno), keypad pins from left to right with keys facing you:
//   Keypad pin 1..7  ->  D9 D8 D7 D6 D5 D4 D3    (check yours: rows R1-R4, cols C1-C3)
//   KY-019 relay  S  -> A0    (drives a 12 V solenoid lock / LED strip)
//   Passive buzzer S -> A1
//   Two-color LED R -> A2, G -> A3 (220 ohm)
//
// Library: Keypad (Mark Stanley, Alexander Brevig)

#include <Keypad.h>
#include <EEPROM.h>

const byte ROWS = 4, COLS = 3;
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
// The common 4x3 membrane pinout is: C2 R1 C1 R4 C3 R3 R2 (7 pins).
// If keys come out wrong, run the test in setup and swap these lists.
byte rowPins[ROWS] = {8, 3, 4, 6};   // R1 R2 R3 R4
byte colPins[COLS] = {7, 9, 5};      // C1 C2 C3
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

const int RELAY_PIN = A0, BUZZER_PIN = A1, LED_R = A2, LED_G = A3;
const unsigned long UNLOCK_MS = 5000;
const int EEPROM_MAGIC_ADDR = 0, EEPROM_PIN_ADDR = 1;
const byte MAGIC = 0x42;

char pin[9] = "1234";        // default PIN, overwritten from EEPROM
char entry[9];
byte entryLen = 0;
int wrongTries = 0;
unsigned long lockoutUntil = 0;
bool changingPin = false;
bool unlocked = false;
unsigned long unlockedAt = 0;

void beep(int f, int ms) { tone(BUZZER_PIN, f, ms); delay(ms + 20); }
void led(bool r, bool g) { digitalWrite(LED_R, r); digitalWrite(LED_G, g); }

void loadPin() {
  if (EEPROM.read(EEPROM_MAGIC_ADDR) != MAGIC) return;      // first run: keep default
  for (int i = 0; i < 9; i++) pin[i] = EEPROM.read(EEPROM_PIN_ADDR + i);
  pin[8] = '\0';
}

void savePin(const char *p) {
  strncpy(pin, p, 8);
  pin[8] = '\0';
  for (int i = 0; i < 9; i++) EEPROM.update(EEPROM_PIN_ADDR + i, pin[i]);  // update = write only if changed
  EEPROM.update(EEPROM_MAGIC_ADDR, MAGIC);
}

void clearEntry() { entryLen = 0; entry[0] = '\0'; }

void lock() {
  unlocked = false;
  digitalWrite(RELAY_PIN, LOW);
  led(false, false);
  Serial.println("Locked.");
}

void unlock() {
  unlocked = true;
  unlockedAt = millis();
  wrongTries = 0;
  digitalWrite(RELAY_PIN, HIGH);
  led(false, true);
  beep(1500, 80); beep(2000, 120);
  Serial.println("UNLOCKED");
}

void wrongPin() {
  wrongTries++;
  led(true, false);
  beep(300, 400);
  // Each wrong try after the third doubles the waiting time: 10 s, 20 s, 40 s...
  if (wrongTries >= 3) {
    unsigned long wait = 10000UL << min(wrongTries - 3, 5);
    lockoutUntil = millis() + wait;
    Serial.print("Too many tries. Locked out for ");
    Serial.print(wait / 1000);
    Serial.println(" s");
  } else {
    Serial.println("Wrong PIN");
  }
  led(false, false);
}

void handleEnter() {
  if (changingPin) {
    if (entryLen >= 4) {
      savePin(entry);
      Serial.println("New PIN saved to EEPROM.");
      beep(2000, 60); beep(2000, 60);
    } else {
      Serial.println("PIN must be 4-8 digits.");
      beep(300, 300);
    }
    changingPin = false;
  } else if (unlocked && strcmp(entry, "*0") == 0) {
    changingPin = true;
    Serial.println("Type the new PIN and press #");
  } else if (strcmp(entry, pin) == 0) {
    unlock();
  } else {
    wrongPin();
  }
  clearEntry();
}

void setup() {
  Serial.begin(9600);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT);
  loadPin();
  lock();
  Serial.println("Keypad lock ready. Press keys to test the layout.");
}

void loop() {
  if (unlocked && !changingPin && millis() - unlockedAt > UNLOCK_MS) lock();

  char k = keypad.getKey();
  if (!k) return;

  if (millis() < lockoutUntil) {               // ignore keys during lockout
    beep(200, 50);
    return;
  }

  beep(1000, 30);                               // key click
  Serial.print("Key: "); Serial.println(k);

  if (k == '#') {
    handleEnter();
  } else if (k == '*' && !(unlocked && entryLen == 0)) {
    clearEntry();                              // '*' clears, except as the first key of '*0#'
  } else if (entryLen < 8) {
    entry[entryLen++] = k;
    entry[entryLen] = '\0';
  }
}
