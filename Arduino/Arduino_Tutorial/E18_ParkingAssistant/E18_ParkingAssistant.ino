// ChinarSoft - Arduino Sensor Kit, Episode 18
// HC-SR04 ultrasonic + passive buzzer + RGB LED + DS18B20 (speed-of-sound correction)
// Project: Garage parking assistant
//   > 100 cm green, 100..40 cm yellow + slow beeps, < 40 cm red + fast beeps, < 15 cm STOP (solid tone)
//   Beeps go quiet after the car has been parked (distance stable) for 10 s
//
// Wiring (Arduino Uno):
//   HC-SR04   VCC -> 5V | TRIG -> D7 | ECHO -> D10 | GND -> GND
//   Passive buzzer S -> D8
//   RGB module R -> D9, G -> D5, B -> D6 (PWM pins, not D3/D11), - -> GND
//   DS18B20 (KY-001) S -> D4
// Libraries: OneWire, DallasTemperature

#include <OneWire.h>
#include <DallasTemperature.h>

const int TRIG_PIN = 7, ECHO_PIN = 10, BUZZER_PIN = 8;
const int LED_R = 9, LED_G = 5, LED_B = 6;   // not D3 or D11: tone() uses Timer2
const int DS_PIN = 4;

const float FAR_CM = 100, NEAR_CM = 40, STOP_CM = 15;
const unsigned long QUIET_AFTER_MS = 10000;

OneWire oneWire(DS_PIN);
DallasTemperature ds(&oneWire);
float soundCmPerUs = 0.0343;    // updated from temperature

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

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT); pinMode(LED_B, OUTPUT);
  ds.begin();
  updateSoundSpeed();
}

void loop() {
  static unsigned long lastTempUpdate = 0, lastBeep = 0, stableSince = 0;
  static float lastCm = 0;

  if (millis() - lastTempUpdate > 60000) { lastTempUpdate = millis(); updateSoundSpeed(); }

  float cm = medianOf5();
  if (cm < 0) cm = 999;                          // no echo = nothing in range

  // Parked = distance stays within 2 cm
  if (fabs(cm - lastCm) > 2) stableSince = millis();
  lastCm = cm;
  bool quiet = millis() - stableSince > QUIET_AFTER_MS;

  unsigned long beepGap;                         // 0 = solid tone, ULONG_MAX = silent
  if (cm > FAR_CM)       { rgb(0, 255, 0);   beepGap = 0xFFFFFFFF; }
  else if (cm > NEAR_CM) { rgb(255, 120, 0); beepGap = map(cm, NEAR_CM, FAR_CM, 250, 800); }
  else if (cm > STOP_CM) { rgb(255, 0, 0);   beepGap = map(cm, STOP_CM, NEAR_CM, 60, 250); }
  else                   { rgb(255, 0, 0);   beepGap = 0; }

  if (quiet || beepGap == 0xFFFFFFFF) noTone(BUZZER_PIN);
  else if (beepGap == 0) tone(BUZZER_PIN, 2500);
  else if (millis() - lastBeep > beepGap) { lastBeep = millis(); tone(BUZZER_PIN, 2500, 40); }

  Serial.print("distance_cm:"); Serial.println(cm > 400 ? 0 : cm, 1);
}
