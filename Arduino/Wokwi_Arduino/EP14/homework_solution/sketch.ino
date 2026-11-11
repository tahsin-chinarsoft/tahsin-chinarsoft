// Episode 14 homework solution - a parking sensor
// Under 50 cm: beeps, faster when closer. Under 10 cm: one steady tone.
const int TRIG = 9;
const int ECHO = 10;
const int BUZZER = 8;

unsigned long lastRead = 0;
unsigned long lastBeep = 0;
float cm = 999;
bool steady = false;

float readCm() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long d = pulseIn(ECHO, HIGH, 30000UL);
  if (d == 0) return 999;                  // nothing in range
  return d * 0.0343 / 2;
}

void setup() {
  Serial.begin(9600);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
}

void loop() {
  // 1) measure every 60 ms
  if (millis() - lastRead >= 60) {
    lastRead = millis();
    cm = readCm();
  }

  // 2) sound
  if (cm < 10) {
    if (!steady) { tone(BUZZER, 1000); steady = true; }      // no duration: keeps playing
  } else {
    if (steady) { noTone(BUZZER); steady = false; }
    if (cm < 50) {
      unsigned long gap = map((long)cm, 10, 50, 100, 600);    // closer = shorter gap
      if (millis() - lastBeep >= gap) {
        lastBeep = millis();
        tone(BUZZER, 1000, 50);                               // 50 ms beep, does not wait
        Serial.print(cm, 1);
        Serial.print(" cm  beep every ");
        Serial.print(gap);
        Serial.println(" ms");
      }
    }
  }
}
