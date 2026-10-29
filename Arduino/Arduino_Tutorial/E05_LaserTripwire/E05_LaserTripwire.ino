// ChinarSoft - Arduino Sensor Kit, Episode 5
// Laser (KY-008), Photoresistor (KY-018), Light interrupter (KY-010), Magic cup (KY-027)
// Project: Laser tripwire alarm
//   - Laser shines on the photoresistor across a doorway
//   - Swipe a card through the KY-010 slot to arm / disarm
//   - Magic cup LED shows the state, tilt the cup to silence the alarm
//
// Wiring (Arduino Uno):
//   KY-008 laser     S -> D7  | - -> GND          (middle pin not connected on most boards)
//   KY-018 LDR       S -> A0  | middle -> 5V | - -> GND
//   KY-010 slot      S -> D2  | middle -> 5V | - -> GND
//   KY-027 magic cup L -> D9 (PWM LED) | S -> D4 (tilt) | + -> 5V | G -> GND
//   Active buzzer    S -> D12 | - -> GND

const int LASER_PIN   = 7;
const int LDR_PIN     = A0;
const int SLOT_PIN    = 2;
const int CUP_LED_PIN = 9;
const int CUP_TILT_PIN = 4;
const int BUZZER_PIN  = 12;

int threshold = 512;
bool laserIsHigh = true;          // does the laser make the reading go UP?
int slotIdle;
int cupIdle;
bool armed = false;
bool alarmOn = false;

int averageLdr(int samples) {
  long sum = 0;
  for (int i = 0; i < samples; i++) { sum += analogRead(LDR_PIN); delay(2); }
  return sum / samples;
}

// Measure with laser off and on, put the threshold in the middle
void calibrate() {
  digitalWrite(LASER_PIN, LOW);
  delay(200);
  int dark = averageLdr(32);
  digitalWrite(LASER_PIN, HIGH);
  delay(200);
  int lit = averageLdr(32);
  threshold = (dark + lit) / 2;
  laserIsHigh = lit > dark;        // KY-018 boards differ in direction
  Serial.print("Ambient: "); Serial.print(dark);
  Serial.print("  Laser: ");  Serial.print(lit);
  Serial.print("  Threshold: "); Serial.println(threshold);
  if (abs(lit - dark) < 100) Serial.println("WARNING: small margin, check laser alignment!");
}

bool beamBroken() {
  int v = analogRead(LDR_PIN);
  return laserIsHigh ? (v < threshold) : (v > threshold);
}

void setArmed(bool a) {
  armed = a;
  alarmOn = false;
  digitalWrite(BUZZER_PIN, LOW);
  if (armed) calibrate();
  Serial.println(armed ? "ARMED" : "DISARMED");
}

void setup() {
  Serial.begin(9600);
  pinMode(LASER_PIN, OUTPUT);
  pinMode(SLOT_PIN, INPUT);
  pinMode(CUP_LED_PIN, OUTPUT);
  pinMode(CUP_TILT_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  delay(100);
  slotIdle = digitalRead(SLOT_PIN);
  cupIdle = digitalRead(CUP_TILT_PIN);
  digitalWrite(LASER_PIN, HIGH);
  Serial.println("Swipe a card through the slot to arm.");
}

void loop() {
  // 1) Card swipe through the KY-010 slot toggles armed state
  static int lastSlot = slotIdle;
  int slot = digitalRead(SLOT_PIN);
  if (slot != slotIdle && lastSlot == slotIdle) {
    setArmed(!armed);
    delay(300);                       // simple debounce for the card edge
  }
  lastSlot = slot;

  // 2) Beam check (needs 3 broken readings in a row = no false alarm from flicker)
  static int brokenCount = 0;
  if (armed && !alarmOn) {
    brokenCount = beamBroken() ? brokenCount + 1 : 0;
    if (brokenCount >= 3) {
      alarmOn = true;
      Serial.println("INTRUDER!");
    }
  }

  // 3) Tilt the magic cup to silence
  if (alarmOn && digitalRead(CUP_TILT_PIN) != cupIdle) {
    alarmOn = false;
    Serial.println("Alarm silenced (still armed)");
    delay(1000);
  }

  // 4) Outputs: buzzer pulses, cup LED = off / slow glow / fast blink
  digitalWrite(BUZZER_PIN, alarmOn && (millis() / 150) % 2);
  int glow;
  if (alarmOn) glow = (millis() / 100) % 2 ? 255 : 0;
  else if (armed) glow = 40 + 40 * sin(millis() / 500.0);
  else glow = 0;
  analogWrite(CUP_LED_PIN, glow);

  delay(5);
}
