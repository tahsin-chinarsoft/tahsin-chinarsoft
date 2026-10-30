// ChinarSoft - Arduino Sensor Kit, Episode 1 - TASK SOLUTION
// Task: three quick presses within 1.5 s play a short five-note song.
//
// Changes vs E01_Doorbell:
//   - Short presses are counted inside a 1.5 s window (firstPressMs / pressCount).
//     The third short press plays a 5-note song with tone().
//   - The ding-dong and the song are now played by a small NON-BLOCKING melody player
//     (updateMelody() in loop). The original dingDong() used delay() for 1.2 s, so the
//     second and third presses would have been missed.
//   - First short press of a window = ding-dong (as before). Presses 2 and 3 are only
//     counted, so you don't hear the ding-dong restart. Press 3 replaces it with the song.
//   - Long press (> 1 s) = urgent beeps, unchanged.
//
// Wiring: same wiring as E01_Doorbell.
//   Button  S -> D2   | middle -> 5V | - -> GND
//   Active buzzer  S -> D8 | - -> GND
//   Passive buzzer S -> D9 | - -> GND
// No library needed.

const int BUTTON_PIN  = 2;
const int ACTIVE_PIN  = 8;
const int PASSIVE_PIN = 9;

const unsigned long DEBOUNCE_MS   = 30;
const unsigned long LONG_PRESS_MS = 1000;
const unsigned long WINDOW_MS     = 1500;   // TASK: 3 presses must fit in this window

int idleLevel;                    // level of the button when NOT pressed (detected at boot)
int lastReading;
int stableState;
unsigned long lastChangeMs = 0;
unsigned long pressStartMs = 0;

unsigned long firstPressMs = 0;   // TASK: start of the current 1.5 s window
int pressCount = 0;               // TASK: short presses inside the window

// Note frequencies in Hz
const int NOTE_C5 = 523;
const int NOTE_D5 = 587;          // TASK: extra notes for the song
const int NOTE_E5 = 659;
const int NOTE_G5 = 784;
const int NOTE_C6 = 1047;

// TASK: melodies = note + how long it sounds + the time until the next note starts
const int  DING_NOTES[] = {NOTE_E5, NOTE_C5};
const int  DING_LEN[]   = {400, 700};
const int  DING_STEP[]  = {450, 750};

const int  SONG_NOTES[] = {NOTE_C5, NOTE_D5, NOTE_E5, NOTE_G5, NOTE_C6};
const int  SONG_LEN[]   = {150, 150, 150, 200, 500};
const int  SONG_STEP[]  = {180, 180, 180, 250, 550};

// TASK: tiny non-blocking melody player
const int *melNotes = 0;
const int *melLen   = 0;
const int *melStep  = 0;
int melCount = 0;
int melIndex = 0;
unsigned long melNoteStart = 0;

void startMelody(const int *notes, const int *len, const int *step, int count) {
  melNotes = notes; melLen = len; melStep = step; melCount = count;
  melIndex = 0;
  melNoteStart = millis();
  tone(PASSIVE_PIN, melNotes[0], melLen[0]);
}

void updateMelody() {
  if (melIndex >= melCount) return;                      // nothing playing
  if (millis() - melNoteStart >= (unsigned long)melStep[melIndex]) {
    melIndex++;
    melNoteStart = millis();
    if (melIndex < melCount) tone(PASSIVE_PIN, melNotes[melIndex], melLen[melIndex]);
    else noTone(PASSIVE_PIN);
  }
}

bool isPressed(int level) {
  return level != idleLevel;
}

void dingDong() {                                        // TASK: now non-blocking
  startMelody(DING_NOTES, DING_LEN, DING_STEP, 2);
}

void playSong() {                                        // TASK: five-note song
  Serial.println("Triple press -> song!");
  startMelody(SONG_NOTES, SONG_LEN, SONG_STEP, 5);
}

void urgentBeeps() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(ACTIVE_PIN, HIGH);   // active buzzer makes its own tone
    delay(100);
    digitalWrite(ACTIVE_PIN, LOW);
    delay(100);
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(ACTIVE_PIN, OUTPUT);
  pinMode(PASSIVE_PIN, OUTPUT);

  // Some KY-004 boards read HIGH when idle, some LOW. Learn it at boot.
  delay(50);
  idleLevel   = digitalRead(BUTTON_PIN);
  lastReading = idleLevel;
  stableState = idleLevel;
  Serial.print("Idle level detected: ");
  Serial.println(idleLevel == HIGH ? "HIGH" : "LOW");
  Serial.println("Ready. Short press = ding-dong, long press = urgent, 3 quick presses = song.");
}

void loop() {
  updateMelody();                     // TASK: keeps the notes going without delay()

  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastReading) {
    lastChangeMs = millis();          // contact is bouncing, restart the timer
    lastReading = reading;
  }

  if (millis() - lastChangeMs > DEBOUNCE_MS && reading != stableState) {
    stableState = reading;

    if (isPressed(stableState)) {
      pressStartMs = millis();
    } else {
      unsigned long held = millis() - pressStartMs;
      Serial.print("Released after ");
      Serial.print(held);
      Serial.println(" ms");
      if (held >= LONG_PRESS_MS) {
        urgentBeeps();
      } else {
        // TASK: on every short release, count presses inside a 1.5 s window
        if (pressCount == 0 || millis() - firstPressMs > WINDOW_MS) {
          firstPressMs = millis();
          pressCount = 0;
        }
        pressCount++;
        Serial.print("Press count: ");
        Serial.println(pressCount);
        if (pressCount == 1) dingDong();
        if (pressCount == 3) {
          playSong();
          pressCount = 0;             // TASK: start fresh, a 4th press is a new ding-dong
        }
      }
    }
  }
}
