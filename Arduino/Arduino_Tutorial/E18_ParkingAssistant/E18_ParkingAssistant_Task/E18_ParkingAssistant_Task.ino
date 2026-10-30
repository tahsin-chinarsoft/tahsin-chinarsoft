// ChinarSoft - Arduino Sensor Kit, Episode 18 - TASK SOLUTION
// Task: set the stop distance with a button press while the car is parked, and save it in EEPROM.
//
// What changed vs E18_ParkingAssistant.ino:
//   - STOP_CM (fixed 15 cm) became the variable stopCm, loaded from EEPROM in setup().
//     Empty/invalid EEPROM -> default 15 cm.
//   - NEW button on D2: park exactly where you want to stop, press -> stopCm = medianOf5(),
//     EEPROM.put(STOP_ADDR, stopCm), confirmation beep 2000 Hz / 200 ms. Power off: it still remembers.
//     No echo / out of range (3..200 cm) -> low error beep, nothing saved.
//   - The yellow/red zones move with stopCm, so a long stop distance still gets its warning zones:
//     red zone = stopCm + 25 cm (at least 40), yellow zone = red + 60 cm (at least 100).
//   - The confirmation beep is protected for 250 ms, so the "parked = quiet" logic can't cut it off.
//
// Wiring: Episode 18 + one push button.
//   HC-SR04   VCC -> 5V | TRIG -> D7 | ECHO -> D10 | GND -> GND
//   Passive buzzer S -> D8
//   RGB module R -> D9, G -> D5, B -> D6 (PWM pins, not D3/D11), - -> GND
//   DS18B20 (KY-001) S -> D4
//   TASK: push button between D2 and GND (internal pull-up). KY-004 module: S -> D2, - -> GND,
//         leave its middle (+) pin unconnected so the pull-up does the work.
// EEPROM: bytes 40..43 = stopCm (float). Episodes 15 (10..13) and 17 (20..35) are not touched.
// Libraries: OneWire, DallasTemperature

#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>                              // TASK

const int TRIG_PIN = 7, ECHO_PIN = 10, BUZZER_PIN = 8;
const int LED_R = 9, LED_G = 5, LED_B = 6;   // not D3 or D11: tone() uses Timer2
const int DS_PIN = 4;
const int BUTTON_PIN = 2;                        // TASK
const int STOP_ADDR = 40;                        // TASK: EEPROM address of stopCm (4 bytes)

const float FAR_CM = 100, NEAR_CM = 40, STOP_CM_DEFAULT = 15;   // TASK: STOP_CM is now a default
const float STOP_MIN_CM = 3, STOP_MAX_CM = 200;                 // TASK: sane range for a learned distance
const unsigned long QUIET_AFTER_MS = 10000;

float stopCm = STOP_CM_DEFAULT;                  // TASK: learned stop distance
float nearCm = NEAR_CM, farCm = FAR_CM;          // TASK: zones follow stopCm

OneWire oneWire(DS_PIN);
DallasTemperature ds(&oneWire);
float soundCmPerUs = 0.0343;    // updated from temperature
unsigned long beepUntil = 0;    // TASK: confirmation beep is playing until this time

// Speed of sound depends on air temperature: v = 331.3 + 0.606 * T  (m/s)
void updateSoundSpeed() {
  ds.requestTemperatures();
  float t = ds.getTempCByIndex(0);
  if (t == DEVICE_DISCONNECTED_C) t = 20.0;     // fallback: room temperature
  soundCmPerUs = (331.3 + 0.606 * t) / 10000.0;
  Serial.print("# Air "); Serial.print(t, 1);
  Serial.print(" C -> sound "); Serial.print(soundCmPerUs * 10000, 1); Serial.println(" m/s");
}

float readCm() {
  digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long us = pulseIn(ECHO_PIN, HIGH, 30000UL);
  if (us == 0) return -1;                        // no echo
  return us * soundCmPerUs / 2.0;
}

float medianOf5() {
  float v[5];
  for (int i = 0; i < 5; i++) { v[i] = readCm(); delay(12); }
  for (int i = 0; i < 4; i++)                    // tiny bubble sort
    for (int j = 0; j < 4 - i; j++)
      if (v[j] > v[j + 1]) { float t = v[j]; v[j] = v[j + 1]; v[j + 1] = t; }
  return v[2];
}

void rgb(int r, int g, int b) { analogWrite(LED_R, r); analogWrite(LED_G, g); analogWrite(LED_B, b); }

// TASK: one 'true' per press, 30 ms debounce, no delay()
bool buttonPressed() {
  static bool stable = HIGH, lastRead = HIGH;
  static unsigned long changedAt = 0;
  bool r = digitalRead(BUTTON_PIN);
  if (r != lastRead) { lastRead = r; changedAt = millis(); }
  if (r != stable && millis() - changedAt > 30) {
    stable = r;
    if (stable == LOW) return true;              // pressed (pull-up: LOW = pressed)
  }
  return false;
}

// TASK: warning zones scale with the stop distance
void updateZones() {
  nearCm = max(NEAR_CM, stopCm + 25);
  farCm  = max(FAR_CM, nearCm + 60);
  Serial.print("# Stop at "); Serial.print(stopCm, 1);
  Serial.print(" cm, red below "); Serial.print(nearCm, 0);
  Serial.print(" cm, yellow below "); Serial.print(farCm, 0); Serial.println(" cm");
}

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);             // TASK
  pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT); pinMode(LED_B, OUTPUT);
  ds.begin();
  updateSoundSpeed();

  EEPROM.get(STOP_ADDR, stopCm);                 // TASK: remember the learned distance
  if (isnan(stopCm) || stopCm < STOP_MIN_CM || stopCm > STOP_MAX_CM) stopCm = STOP_CM_DEFAULT;  // empty EEPROM = 0xFF.. = NaN
  updateZones();
}

void loop() {
  static unsigned long lastTempUpdate = 0, lastBeep = 0, stableSince = 0;
  static float lastCm = 0;

  if (millis() - lastTempUpdate > 60000) { lastTempUpdate = millis(); updateSoundSpeed(); }

  // TASK: car parked, button pressed -> learn the stop distance
  if (buttonPressed()) {
    float cm = medianOf5();
    if (cm >= STOP_MIN_CM && cm <= STOP_MAX_CM) {
      stopCm = cm;
      EEPROM.put(STOP_ADDR, stopCm);             // put() only writes bytes that changed
      tone(BUZZER_PIN, 2000, 200);               // confirmation beep
      updateZones();
    } else {
      tone(BUZZER_PIN, 400, 400);                // error: no echo or out of range, nothing saved
      Serial.println("# No valid distance - stop distance NOT changed");
    }
    beepUntil = millis() + 450;
  }

  float cm = medianOf5();
  if (cm < 0) cm = 999;                          // no echo = nothing in range

  // Parked = distance stays within 2 cm
  if (fabs(cm - lastCm) > 2) stableSince = millis();
  lastCm = cm;
  bool quiet = millis() - stableSince > QUIET_AFTER_MS;

  unsigned long beepGap;                         // 0 = solid tone, ULONG_MAX = silent
  if (cm > farCm)        { rgb(0, 255, 0);   beepGap = 0xFFFFFFFF; }
  else if (cm > nearCm)  { rgb(255, 120, 0); beepGap = map(cm, nearCm, farCm, 250, 800); }
  else if (cm > stopCm)  { rgb(255, 0, 0);   beepGap = map(cm, stopCm, nearCm, 60, 250); }
  else                   { rgb(255, 0, 0);   beepGap = 0; }

  if ((long)(millis() - beepUntil) < 0) { /* TASK: let the confirmation beep finish */ }
  else if (quiet || beepGap == 0xFFFFFFFF) noTone(BUZZER_PIN);
  else if (beepGap == 0) tone(BUZZER_PIN, 2500);
  else if (millis() - lastBeep > beepGap) { lastBeep = millis(); tone(BUZZER_PIN, 2500, 40); }

  Serial.print("distance_cm:"); Serial.println(cm > 400 ? 0 : cm, 1);
}
