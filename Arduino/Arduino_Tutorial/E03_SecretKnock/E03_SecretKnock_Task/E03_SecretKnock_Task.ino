// ChinarSoft - Arduino Sensor Kit, Episode 3 - TASK SOLUTION
// Task: after three wrong knock patterns, lock out for 30 s and blink red.
//
// Changes vs E03_SecretKnock:
//   - wrongTries counts wrong patterns; the 3rd one stores lockedUntil = now + 30 s.
//   - While locked: the LED blinks red, every knock is ignored, 'L' (learn) is refused.
//     The tilt (tamper) alarm keeps working.
//   - A correct knock resets wrongTries. When the lockout ends, wrongTries resets too,
//     so you get three fresh tries.
//   - The time check is written as (long)(millis() - lockedUntil) < 0 plus a 'locked'
//     flag, so it stays correct when millis() rolls over after ~49 days.
//
// Wiring: same wiring as E03_SecretKnock.
//
// Tap (KY-031), Vibration switch (KY-002), Tilt switches (KY-020 / KY-017 / other)
// Project: Secret-knock lock with a tamper (tilt) alarm
//
// Wiring (Arduino Uno):
//   Tap sensor KY-031     S -> D2   | middle -> 5V | - -> GND
//   Vibration KY-002      S -> D3   | middle -> 5V | - -> GND
//   Tilt switch A         S -> D4   | middle -> 5V | - -> GND
//   Tilt switch B         S -> D7
//   Tilt switch C         S -> D8
//   Two-color LED         R -> D5, G -> D6 (220 ohm each), - -> GND
//   Active buzzer         S -> D12, - -> GND
//
// Serial: 'L' = learn a new knock pattern, 'T' = print tilt switch states

const int TAP_PIN = 2;
const int VIB_PIN = 3;
const int TILT_PINS[3] = {4, 7, 8};
const int LED_R = 5, LED_G = 6;
const int BUZZER_PIN = 12;

int KNOCK_PIN = TAP_PIN;              // change to VIB_PIN to compare sensors

const int MAX_KNOCKS = 10;
const unsigned long KNOCK_LOCKOUT_MS = 80;    // ignore ringing after a knock
const unsigned long PATTERN_END_MS   = 1200;  // silence that ends a pattern
const int TOLERANCE_PCT = 25;                 // allowed timing error per gap

// Stored secret: "shave and a haircut" style, gaps in relative units (0..100)
int secret[MAX_KNOCKS] = {50, 25, 25, 50, 100, 50};
int secretCount = 7;                  // number of knocks (gaps = knocks - 1)

unsigned long knockTimes[MAX_KNOCKS];
int knockCount = 0;
bool learning = false;
int tiltIdle[3];
int knockIdle;

const int MAX_WRONG_TRIES = 3;                // TASK
const unsigned long LOCKOUT_MS = 30000;       // TASK: 30 s
int wrongTries = 0;                           // TASK: wrong patterns in a row
bool locked = false;                          // TASK
unsigned long lockedUntil = 0;                // TASK: end of the lockout

void setLed(bool red, bool green) {
  digitalWrite(LED_R, red ? HIGH : LOW);
  digitalWrite(LED_G, green ? HIGH : LOW);
}

// Turn absolute times into gaps scaled so the longest gap = 100
int normalize(unsigned long *times, int count, int *gaps) {
  unsigned long longest = 1;
  for (int i = 1; i < count; i++) longest = max(longest, times[i] - times[i - 1]);
  for (int i = 1; i < count; i++) gaps[i - 1] = (times[i] - times[i - 1]) * 100 / longest;
  return count - 1;
}

bool patternMatches() {
  if (knockCount != secretCount) return false;
  int gaps[MAX_KNOCKS];
  int n = normalize(knockTimes, knockCount, gaps);
  for (int i = 0; i < n; i++) {
    if (abs(gaps[i] - secret[i]) > TOLERANCE_PCT) return false;
  }
  return true;
}

void finishPattern() {
  Serial.print("Pattern with ");
  Serial.print(knockCount);
  Serial.println(" knocks.");

  if (learning) {
    secretCount = knockCount;
    int n = normalize(knockTimes, knockCount, secret);
    Serial.print("Saved gaps:");
    for (int i = 0; i < n; i++) { Serial.print(' '); Serial.print(secret[i]); }
    Serial.println();
    learning = false;
    setLed(false, false);
  } else if (patternMatches()) {
    Serial.println("UNLOCKED");
    wrongTries = 0;                           // TASK: correct knock resets the counter
    setLed(false, true);
    delay(2000);
    setLed(false, false);
  } else {
    Serial.println("Wrong knock");
    if (++wrongTries >= MAX_WRONG_TRIES) {    // TASK: third wrong try -> lock out
      locked = true;
      lockedUntil = millis() + LOCKOUT_MS;
      Serial.println("LOCKED OUT for 30 s");
    } else {
      Serial.print("Tries left: ");
      Serial.println(MAX_WRONG_TRIES - wrongTries);
      setLed(true, false);
      delay(1000);
      setLed(false, false);
    }
  }
  knockCount = 0;
}

void checkTilt() {
  for (int i = 0; i < 3; i++) {
    if (digitalRead(TILT_PINS[i]) != tiltIdle[i]) {
      Serial.print("TAMPER! Tilt switch ");
      Serial.println(i);
      for (int k = 0; k < 3; k++) {
        digitalWrite(BUZZER_PIN, HIGH); delay(150);
        digitalWrite(BUZZER_PIN, LOW);  delay(100);
      }
      return;
    }
  }
}

void handleSerial() {
  if (!Serial.available()) return;
  char c = Serial.read();
  if ((c == 'L' || c == 'l') && locked) {      // TASK: no re-learning during a lockout
    Serial.println("Locked out - try again later");
  } else if (c == 'L' || c == 'l') {
    learning = true;
    knockCount = 0;
    setLed(true, true);              // yellow = learning
    Serial.println("Learning: knock your new pattern...");
  } else if (c == 'T' || c == 't') {
    for (int i = 0; i < 3; i++) {
      Serial.print("Tilt "); Serial.print(i); Serial.print(": ");
      Serial.println(digitalRead(TILT_PINS[i]));
    }
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(TAP_PIN, INPUT);
  pinMode(VIB_PIN, INPUT);
  for (int i = 0; i < 3; i++) pinMode(TILT_PINS[i], INPUT);
  pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  delay(100);
  for (int i = 0; i < 3; i++) tiltIdle[i] = digitalRead(TILT_PINS[i]);  // box must sit level at boot
  knockIdle = digitalRead(KNOCK_PIN);
  Serial.println("Knock lock ready. 'L' = learn, 'T' = tilt states");
}

void loop() {
  handleSerial();

  // TASK: lockout - blink red, ignore knocks, keep the tamper alarm alive
  if (locked) {
    if ((long)(millis() - lockedUntil) < 0) {
      setLed((millis() / 250) % 2, false);    // red blinks 2x per second
      checkTilt();
      return;                                 // knocks are not read at all
    }
    locked = false;
    wrongTries = 0;
    setLed(false, false);
    Serial.println("Lockout over, you have 3 new tries");
  }

  static unsigned long lastKnock = 0;
  if (digitalRead(KNOCK_PIN) != knockIdle && millis() - lastKnock > KNOCK_LOCKOUT_MS) {
    lastKnock = millis();
    if (knockCount < MAX_KNOCKS) knockTimes[knockCount++] = lastKnock;
    digitalWrite(LED_G, HIGH); delay(10); digitalWrite(LED_G, learning ? HIGH : LOW);
  }

  if (knockCount > 0 && millis() - lastKnock > PATTERN_END_MS) finishPattern();

  if (knockCount == 0) checkTilt();
}
