// ChinarSoft - Arduino Sensor Kit, Episode 1
// Button (KY-004) + Active Buzzer (KY-012) + Passive Buzzer (KY-006): Smart Doorbell
//
// Wiring (Arduino Uno):
//   Button  S -> D2   | middle -> 5V | - -> GND
//   Active buzzer  S -> D8 | - -> GND
//   Passive buzzer S -> D9 | - -> GND
//
// Short press : "ding-dong" on the passive buzzer
// Long press  : (> 1 s) urgent beeps on the active buzzer
// No library needed.

const int BUTTON_PIN  = 2;
const int ACTIVE_PIN  = 8;
const int PASSIVE_PIN = 9;

const unsigned long DEBOUNCE_MS   = 30;
const unsigned long LONG_PRESS_MS = 1000;

int idleLevel;                    // level of the button when NOT pressed (detected at boot)
int lastReading;
int stableState;
unsigned long lastChangeMs = 0;
unsigned long pressStartMs = 0;

// Note frequencies in Hz
const int NOTE_E5 = 659;
const int NOTE_C5 = 523;

bool isPressed(int level) {
  return level != idleLevel;
}

void dingDong() {
  tone(PASSIVE_PIN, NOTE_E5, 400);
  delay(450);
  tone(PASSIVE_PIN, NOTE_C5, 700);
  delay(750);
  noTone(PASSIVE_PIN);
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
  Serial.println("Ready. Short press = ding-dong, long press = urgent.");
}

void loop() {
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
      if (held >= LONG_PRESS_MS) urgentBeeps();
      else dingDong();
    }
  }
}
