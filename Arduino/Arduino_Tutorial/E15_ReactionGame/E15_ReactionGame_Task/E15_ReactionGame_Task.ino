// ChinarSoft - Arduino Sensor Kit, Episode 15 - TASK SOLUTION
// Task: a two-player mode - player 1 on the buttons, player 2 on the joystick, first correct hit wins.
//
// What changed vs E15_ReactionGame.ino:
//   - The single-player game (press E) and its EEPROM record are unchanged.
//   - NEW: press K (joystick push) to start a two-player match of 5 rounds.
//   - buttonTarget() reads ONLY buttons A-D, stickTarget() reads ONLY the joystick directions.
//     Both return a slot number 0..3, so one random target works for both players:
//       slot 0 = A / UP,  1 = B / DOWN,  2 = C / LEFT,  3 = D / RIGHT
//     At GO the Serial Monitor shows the letter for player 1 and the direction for player 2
//     (the "fair split": each player always has his own target in every round).
//   - First correct hit wins the round. False start before GO = the other player gets the round.
//     A wrong hit after GO locks that player out for 500 ms.
//   - Two-player results are NOT written to EEPROM (the record stays a single-player record).
//
// Wiring: identical to Episode 15, no extra parts.
//   A -> D2  B -> D3  C -> D4  D -> D5  E -> D6  F -> D7  Joystick push (K) -> D8
//   X -> A0  Y -> A1    Set the shield's voltage switch to 5V for the Uno.
// On-board LED D13 lights at "GO".
// EEPROM: bytes 10..13 = best single-player average (unchanged).

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

// TASK: two-player slots. Slot i = letter TARGET_NAMES[i] for P1, direction TARGET_NAMES[i + 4] for P2
const int SLOTS = 4;
const unsigned long LOCKOUT_MS = 500;   // TASK: wrong hit = 500 ms lockout for that player

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

// TASK: player 1 - buttons A-D only. Returns slot 0..3 or -1
int buttonTarget() {
  for (int i = 0; i < 4; i++) if (digitalRead(BTN_PINS[i]) == LOW) return i;
  return -1;
}

// TASK: player 2 - joystick directions only. Returns slot 0..3 or -1
int stickTarget() {
  int x = analogRead(JOY_X) - 512;
  int y = analogRead(JOY_Y) - 512;
  if (y >  JOY_LIMIT) return T_UP - T_UP;       // 0
  if (y < -JOY_LIMIT) return T_DOWN - T_UP;     // 1
  if (x < -JOY_LIMIT) return T_LEFT - T_UP;     // 2
  if (x >  JOY_LIMIT) return T_RIGHT - T_UP;    // 3
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

// TASK: one two-player round. Returns the winner: 1 or 2
int playRound2P(int round) {
  int target = random(SLOTS);
  Serial.print("\nRound "); Serial.print(round);
  Serial.println(" - both hands off... get ready");

  waitForRelease();
  unsigned long waitStart = millis();
  unsigned long waitMs = random(1500, 4000);

  // Waiting phase: a false start gives the round to the other player (catches the cheater)
  while (millis() - waitStart < waitMs) {
    if (buttonTarget() != -1) { Serial.println("P1 FALSE START -> round to Player 2"); waitForRelease(); return 2; }
    if (stickTarget()  != -1) { Serial.println("P2 FALSE START -> round to Player 1"); waitForRelease(); return 1; }
  }

  digitalWrite(LED_PIN, HIGH);
  Serial.print(">>> GO!  P1: press "); Serial.print(TARGET_NAMES[target]);
  Serial.print("   P2: push "); Serial.println(TARGET_NAMES[target + T_UP]);
  unsigned long start = micros();
  unsigned long p1LockedAt = 0, p2LockedAt = 0;
  bool p1Locked = false, p2Locked = false;
  int winner = 0;

  while (winner == 0) {
    int p1 = buttonTarget();   // A-D only
    int p2 = stickTarget();    // directions only
    if (p1Locked && millis() - p1LockedAt >= LOCKOUT_MS && p1 == -1) p1Locked = false;
    if (p2Locked && millis() - p2LockedAt >= LOCKOUT_MS && p2 == -1) p2Locked = false;
    if (p1Locked) p1 = -1;
    if (p2Locked) p2 = -1;

    if (p1 == target) winner = 1;
    else if (p2 == target) winner = 2;
    else {
      if (p1 != -1) { p1Locked = true; p1LockedAt = millis(); Serial.println("P1 wrong - locked 0.5 s"); }
      if (p2 != -1) { p2Locked = true; p2LockedAt = millis(); Serial.println("P2 wrong - locked 0.5 s"); }
    }
  }
  unsigned long ms = (micros() - start) / 1000;
  digitalWrite(LED_PIN, LOW);
  Serial.print("Player "); Serial.print(winner);
  Serial.print(" wins the round in "); Serial.print(ms); Serial.println(" ms");
  return winner;
}

// TASK: a full two-player match
void playMatch2P() {
  Serial.println("\n=== TWO-PLAYER MODE: Player 1 = buttons A-D, Player 2 = joystick ===");
  int wins[3] = {0, 0, 0};
  for (int r = 1; r <= ROUNDS; r++) {
    wins[playRound2P(r)]++;
    Serial.print("Score  P1 "); Serial.print(wins[1]);
    Serial.print(" : "); Serial.print(wins[2]); Serial.println(" P2");
  }
  Serial.print("\n*** PLAYER "); Serial.print(wins[1] > wins[2] ? 1 : 2);   // 5 rounds: no draw
  Serial.println(" WINS THE MATCH! ***");
  Serial.println("Press E for single player, K for two players.");
}

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 7; i++) pinMode(BTN_PINS[i], INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  randomSeed(analogRead(A5));                  // floating pin = some randomness
  Serial.println("REACTION GAME - E = single player, K = two players, F = reset the record");  // TASK
}

void loop() {
  if (digitalRead(BTN_PINS[5]) == LOW) {        // F = reset record
    unsigned long none = 99999;
    EEPROM.put(EEPROM_ADDR, none);
    Serial.println("Record cleared.");
    delay(500);
  }

  if (digitalRead(BTN_PINS[6]) == LOW) {        // TASK: K = two-player match
    while (digitalRead(BTN_PINS[6]) == LOW) delay(5);   // wait until K is released
    playMatch2P();
    delay(500);
    return;
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
