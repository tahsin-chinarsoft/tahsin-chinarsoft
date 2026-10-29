// ChinarSoft - Arduino Sensor Kit, Episode 15
// Joystick Shield (Funduino style): buttons A-F + joystick with push button
// Project: Reaction-time game, 5 rounds, best score saved in EEPROM
//   At GO the Serial Monitor shows a target: a button letter (A/B/C/D) or a joystick
//   direction (UP/DOWN/LEFT/RIGHT). Hit it as fast as you can.
//   Pressing before "GO" = false start (+500 ms penalty).
//
// Shield pinout (fixed, typical):
//   A -> D2  B -> D3  C -> D4  D -> D5  E -> D6  F -> D7  Joystick push (K) -> D8
//   X -> A0  Y -> A1
//   Set the shield's voltage switch to 5V for the Uno.
// On-board LED D13 lights at "GO".

#include <EEPROM.h>

const int BTN_PINS[7] = {2, 3, 4, 5, 6, 7, 8};
const char *BTN_NAMES[7] = {"A", "B", "C", "D", "E", "F", "K"};
const int JOY_X = A0, JOY_Y = A1;
const int LED_PIN = 13;
const int ROUNDS = 5;
const int JOY_LIMIT = 300;            // how far from center counts as a direction
const int EEPROM_ADDR = 10;           // 4 bytes: best average in ms

enum Target { T_A, T_B, T_C, T_D, T_UP, T_DOWN, T_LEFT, T_RIGHT, T_COUNT };
const char *TARGET_NAMES[T_COUNT] = {"A", "B", "C", "D", "UP", "DOWN", "LEFT", "RIGHT"};

// Returns what the player is doing right now, or -1 for nothing
int currentInput() {
  for (int i = 0; i < 4; i++) if (digitalRead(BTN_PINS[i]) == LOW) return T_A + i;
  int x = analogRead(JOY_X) - 512;
  int y = analogRead(JOY_Y) - 512;
  if (y >  JOY_LIMIT) return T_UP;
  if (y < -JOY_LIMIT) return T_DOWN;
  if (x < -JOY_LIMIT) return T_LEFT;
  if (x >  JOY_LIMIT) return T_RIGHT;
  return -1;
}

void waitForRelease() {
  while (currentInput() != -1) delay(5);
  delay(30);
}

unsigned long readBest() {
  unsigned long best;
  EEPROM.get(EEPROM_ADDR, best);
  if (best == 0 || best == 0xFFFFFFFF) best = 99999;     // empty EEPROM
  return best;
}

unsigned long playRound(int round) {
  int target = random(T_COUNT);
  Serial.print("\nRound "); Serial.print(round);
  Serial.println(" - get ready...");

  waitForRelease();
  unsigned long goAt = millis() + random(1500, 4000);
  unsigned long penalty = 0;

  while (millis() < goAt) {                    // waiting phase: catch false starts
    if (currentInput() != -1) {
      Serial.println("FALSE START! +500 ms");
      penalty += 500;
      waitForRelease();
    }
  }

  digitalWrite(LED_PIN, HIGH);
  Serial.print(">>> GO: ");             // target is revealed only now
  Serial.println(TARGET_NAMES[target]);
  unsigned long start = micros();
  int in;
  while (true) {
    in = currentInput();
    if (in == target) break;
    if (in != -1) {                            // wrong input: small penalty, keep going
      penalty += 200;
      Serial.print("Wrong ("); Serial.print(TARGET_NAMES[in]); Serial.println(") +200 ms");
      waitForRelease();
    }
  }
  unsigned long ms = (micros() - start) / 1000 + penalty;
  digitalWrite(LED_PIN, LOW);
  Serial.print("Time: "); Serial.print(ms); Serial.println(" ms");
  return ms;
}

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 7; i++) pinMode(BTN_PINS[i], INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  randomSeed(analogRead(A5));                  // floating pin = some randomness
  Serial.println("REACTION GAME - press E to start a game, F to reset the record");
}

void loop() {
  if (digitalRead(BTN_PINS[5]) == LOW) {        // F = reset record
    unsigned long none = 99999;
    EEPROM.put(EEPROM_ADDR, none);
    Serial.println("Record cleared.");
    delay(500);
  }
  if (digitalRead(BTN_PINS[4]) != LOW) return;  // wait for E

  unsigned long total = 0;
  for (int r = 1; r <= ROUNDS; r++) total += playRound(r);
  unsigned long avg = total / ROUNDS;
  unsigned long best = readBest();

  Serial.print("\nAverage: "); Serial.print(avg); Serial.println(" ms");
  if (avg < best) {
    EEPROM.put(EEPROM_ADDR, avg);
    Serial.println("*** NEW RECORD! ***");
  } else {
    Serial.print("Record to beat: "); Serial.print(best); Serial.println(" ms");
  }
  Serial.println("Press E to play again.");
  delay(500);
}
