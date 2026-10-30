// ChinarSoft - Arduino Sensor Kit, Episode 8 - TASK SOLUTION
// Task: flag a "jam" if two parts arrive less than 1 second apart.
//
// What changed vs E08_IRStation:
//   - New variable lastPartMs remembers when the last part arrived.
//   - On every new part: if it came less than JAM_MS (1000 ms) after the previous one,
//     print "JAM!", count it, and light the RED LED. Red stays on until the next
//     normal (non-jam) part, so the operator can see it. Then lastPartMs = now.
//   - Fire interlock, part typing (A/B) and counters unchanged.
//
// Wiring: same wiring as E08_IRStation (no extra parts):
//   KY-032 obstacle   OUT -> D2 | + -> 5V | GND -> GND | EN: leave jumper ON
//   KY-033 tracking   S -> D3   | V+ -> 5V | G -> GND
//   KY-026 flame      AO -> A0  | DO -> D4 | + -> 5V | G -> GND
//   Two-color LED     R -> D5, G -> D6 (220 ohm)
//   Active buzzer     S -> D12

const int OBSTACLE_PIN = 2;   // LOW = object detected (most KY-032 boards)
const int TRACK_PIN    = 3;   // HIGH = black / no reflection (most KY-033 boards)
const int FLAME_AO     = A0;
const int FLAME_DO     = 4;
const int LED_R = 5, LED_G = 6;
const int BUZZER_PIN   = 12;

const int FLAME_LIMIT = 300;  // analog: LOWER value = MORE infrared on most KY-026 boards
const unsigned long JAM_MS = 1000;   // TASK: two parts closer than this = jam

unsigned long countA = 0, countB = 0;
unsigned long jamCount = 0;          // TASK
unsigned long lastPartMs = 0;        // TASK: when the last part arrived
bool havePart = false;               // TASK: no "previous part" yet after power-up
bool partPresent = false;
bool fireStop = false;

void led(bool r, bool g) { digitalWrite(LED_R, r); digitalWrite(LED_G, g); }

void setup() {
  Serial.begin(9600);
  pinMode(OBSTACLE_PIN, INPUT);
  pinMode(TRACK_PIN, INPUT);
  pinMode(FLAME_DO, INPUT);
  pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  led(false, true);
  Serial.println("Station running.");
}

void loop() {
  int flame = analogRead(FLAME_AO);

  // --- Safety first: fire interlock latches until reset over serial ('r') ---
  if (!fireStop && flame < FLAME_LIMIT) {
    fireStop = true;
    Serial.print("FIRE STOP! flame sensor = ");
    Serial.println(flame);
  }
  if (fireStop) {
    led((millis() / 200) % 2, false);
    digitalWrite(BUZZER_PIN, (millis() / 200) % 2);
    if (Serial.available() && Serial.read() == 'r' && flame >= FLAME_LIMIT) {
      fireStop = false;
      digitalWrite(BUZZER_PIN, LOW);
      led(false, true);
      Serial.println("Reset. Station running.");
    }
    return;                    // nothing else runs while stopped
  }

  // --- Part detection on the rising edge of "object present" ---
  bool present = digitalRead(OBSTACLE_PIN) == LOW;
  if (present && !partPresent) {
    unsigned long now = millis();                      // TASK: arrival time
    bool jam = havePart && (now - lastPartMs < JAM_MS); // TASK: too soon after the last one?

    delay(50);                 // let the part settle under the line sensor
    bool black = digitalRead(TRACK_PIN) == HIGH;
    if (black) countB++; else countA++;
    led(true, true);           // yellow flash = part logged
    Serial.print("Part type ");
    Serial.print(black ? "B (black mark)" : "A (white)");
    Serial.print(" | A="); Serial.print(countA);
    Serial.print(" B=");   Serial.print(countB);
    Serial.print(" | flame AO="); Serial.println(flame);
    delay(100);

    if (jam) {                 // TASK: something upstream is stuck
      jamCount++;
      Serial.print("JAM! only "); Serial.print(now - lastPartMs);
      Serial.print(" ms since last part | jams="); Serial.println(jamCount);
      led(true, false);        // red stays on until the next normal part
    } else {
      led(false, true);
    }
    lastPartMs = now;          // TASK
    havePart = true;           // TASK
  }
  partPresent = present;
}
