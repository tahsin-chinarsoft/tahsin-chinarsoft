// ChinarSoft - Arduino Sensor Kit, Episode 17
// MQ gas sensors: MQ-3 (alcohol), MQ-4 (methane/CNG), MQ-7 (carbon monoxide), MQ-135 (air quality)
// Project: 4-channel gas monitor with warm-up, clean-air calibration and relative alarms
//
// !!! These are hobby sensors. NOT a certified gas/CO detector. Never rely on them for safety. !!!
//
// POWER: each heater draws ~150 mA. Four sensors = ~600 mA -> more than USB can give.
//        Power the sensors from a separate 5 V / 2 A supply and connect its GND to Arduino GND.
//
// Wiring (Arduino Uno):
//   MQ-3   AO -> A0     MQ-4   AO -> A1
//   MQ-7   AO -> A2     MQ-135 AO -> A3
//   All VCC -> external 5 V, all GND -> external GND + Arduino GND
//   Active buzzer S -> D12
//
// Serial (115200): 'c' = calibrate in clean air (saves R0 to EEPROM)
// Output: ratio Rs/R0 per sensor. 1.00 = clean air, lower = more gas.

#include <EEPROM.h>

const int N = 4;
const char *NAMES[N]   = {"MQ3_alcohol", "MQ4_methane", "MQ7_CO", "MQ135_air"};
const int PINS[N]      = {A0, A1, A2, A3};
const float RL_KOHM[N] = {1.0, 1.0, 1.0, 1.0};   // load resistor on YOUR board (often 1k, sometimes 10k: measure it)
const float ALARM_RATIO[N] = {0.5, 0.6, 0.6, 0.6};  // alarm when Rs/R0 falls below this
const unsigned long WARMUP_MS = 3UL * 60UL * 1000UL; // 3 min minimum; 24-48 h burn-in for new sensors
const int BUZZER_PIN = 12;
const int EEPROM_ADDR = 20;

float r0[N] = {10, 10, 10, 10};   // sensor resistance in clean air (kOhm)

// Sensor resistance from the analog voltage:
// Vout = Vc * RL / (Rs + RL)  ->  Rs = RL * (Vc - Vout) / Vout
float readRs(int i) {
  long sum = 0;
  for (int k = 0; k < 32; k++) sum += analogRead(PINS[i]);
  float raw = sum / 32.0;
  if (raw < 1) raw = 1;                      // avoid divide by zero
  return RL_KOHM[i] * (1023.0 - raw) / raw;
}

void calibrate() {
  Serial.println("Calibrating in CLEAN AIR, 20 s...");
  float sum[N] = {0, 0, 0, 0};
  for (int s = 0; s < 20; s++) {
    for (int i = 0; i < N; i++) sum[i] += readRs(i);
    delay(1000);
  }
  for (int i = 0; i < N; i++) r0[i] = sum[i] / 20.0;
  EEPROM.put(EEPROM_ADDR, r0);
  Serial.println("R0 saved.");
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  float saved[N];
  EEPROM.get(EEPROM_ADDR, saved);
  if (!isnan(saved[0]) && saved[0] > 0.1 && saved[0] < 1000) {
    for (int i = 0; i < N; i++) r0[i] = saved[i];
    Serial.println("R0 loaded from EEPROM.");
  } else {
    Serial.println("No calibration yet: after warm-up, press 'c' in clean air.");
  }
}

void loop() {
  // 1) Warm-up: readings are meaningless while the heater is still heating
  if (millis() < WARMUP_MS) {
    static unsigned long lastMsg = 0;
    if (millis() - lastMsg > 10000) {
      lastMsg = millis();
      Serial.print("# Warming up... ");
      Serial.print((WARMUP_MS - millis()) / 1000);
      Serial.println(" s left");
    }
    return;
  }

  if (Serial.available() && Serial.read() == 'c') calibrate();

  // 2) Read all sensors, print ratios for the Serial Plotter
  bool alarm = false;
  for (int i = 0; i < N; i++) {
    float ratio = readRs(i) / r0[i];
    if (ratio < ALARM_RATIO[i]) alarm = true;
    Serial.print(NAMES[i]); Serial.print(':'); Serial.print(ratio, 2);
    Serial.print(i < N - 1 ? ' ' : '\n');
  }

  digitalWrite(BUZZER_PIN, alarm && (millis() / 250) % 2);
  delay(500);
}
