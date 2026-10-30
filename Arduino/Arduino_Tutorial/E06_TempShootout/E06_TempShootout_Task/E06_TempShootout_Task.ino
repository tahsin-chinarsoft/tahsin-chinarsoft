// ChinarSoft - Arduino Sensor Kit, Episode 6 - TASK SOLUTION
// Task: calibrate the KY-013 NTC with a two-point correction, using the DS18B20 as reference.
//
// Changes vs E06_TempShootout:
//   - Two-point calibration from the Serial Monitor (115200 baud, any line ending):
//       c = store the COLD point (both sensors together in ice water, wait ~1 min)
//       h = store the HOT point  (both sensors in warm water, ~40-50 C, wait ~1 min)
//       x = forget the calibration (gain 1, offset 0)
//     With both points: gain = (refHot - refCold) / (ntcHot - ntcCold)
//                       offset = refCold - gain * ntcCold
//   - New plot line KY013_cal: = gain * tNtc + offset (corrected NTC reading).
//   - gain and offset are saved in EEPROM, so the calibration survives a reset.
//   - Waterproof the sensors (or use a sealed DS18B20 probe) before putting them in water!
//
// Wiring: same wiring as E06_TempShootout.
//
// Temperature shoot-out: KY-013 analog NTC, KY-028 digital NTC, KY-001 DS18B20, KY-015 DHT11
//
// Wiring (Arduino Uno):
//   KY-013 analog temp    S -> A0 | middle -> 5V | - -> GND
//   KY-028 digital temp   AO -> A1 | DO -> D3 | + -> 5V | G -> GND
//   KY-001 DS18B20        S -> D4 | middle -> 5V | - -> GND   (board has a 4.7k pull-up)
//   KY-015 DHT11          S -> D5 | middle -> 5V | - -> GND
//
// Libraries: OneWire, DallasTemperature, DHT sensor library (Adafruit) + Adafruit Unified Sensor
// Output: Serial Plotter at 115200 baud

#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <EEPROM.h>                     // TASK: keep the calibration after a reset

const int NTC_PIN     = A0;
const int KY028_AO    = A1;
const int KY028_DO    = 3;
const int DS18B20_PIN = 4;
const int DHT_PIN     = 5;

// NTC parameters (typical 10k NTC, B = 3950, 10k fixed resistor)
const float R_FIXED  = 10000.0;
const float R_NOMINAL = 10000.0;
const float T_NOMINAL = 25.0;
const float B_COEFF  = 3950.0;
// Many KY-013 boards have the NTC and the resistor swapped vs. the label.
// If the temperature goes DOWN when you warm the sensor, flip this.
const bool NTC_ON_GROUND_SIDE = true;

OneWire oneWire(DS18B20_PIN);
DallasTemperature ds(&oneWire);
DHT dht(DHT_PIN, DHT11);

// TASK: two-point calibration data
float refCold = NAN, ntcCold = NAN;     // ice water: DS18B20 and NTC readings
float refHot  = NAN, ntcHot  = NAN;     // warm water
float gain = 1.0, offset = 0.0;         // corrected = gain * tNtc + offset
const float MIN_SPAN_C = 10.0;          // the two points must be at least 10 C apart

const int  EE_ADDR  = 0;
const long EE_MAGIC = 0x4E544331;       // "NTC1": marks valid data in EEPROM
struct CalData { long magic; float gain; float offset; };

float ntcCelsius(int raw, bool ntcOnGroundSide) {
  if (raw <= 0 || raw >= 1023) return NAN;
  float r;
  if (ntcOnGroundSide) r = R_FIXED * raw / (1023.0 - raw);
  else                 r = R_FIXED * (1023.0 - raw) / raw;
  // Beta form of the Steinhart-Hart equation
  float invT = 1.0 / (T_NOMINAL + 273.15) + log(r / R_NOMINAL) / B_COEFF;
  return 1.0 / invT - 273.15;
}

// TASK: compute the straight-line correction from the two points
void computeCalibration() {
  if (isnan(refCold) || isnan(refHot)) return;      // need both points
  if (fabs(ntcHot - ntcCold) < MIN_SPAN_C) {
    Serial.println("# Points too close: use ice water and clearly warmer water");
    return;
  }
  gain = (refHot - refCold) / (ntcHot - ntcCold);
  offset = refCold - gain * ntcCold;
  CalData d = {EE_MAGIC, gain, offset};
  EEPROM.put(EE_ADDR, d);
  Serial.print("# Calibrated and saved: gain = "); Serial.print(gain, 4);
  Serial.print("  offset = "); Serial.println(offset, 3);
}

// TASK: 'c' / 'h' store a point from the current readings, 'x' clears
void handleSerial(float tDs, float tNtc) {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'x' || c == 'X') {
      gain = 1.0; offset = 0.0;
      refCold = ntcCold = refHot = ntcHot = NAN;
      CalData d = {0, 1.0, 0.0};
      EEPROM.put(EE_ADDR, d);
      Serial.println("# Calibration cleared");
    } else if (c == 'c' || c == 'C' || c == 'h' || c == 'H') {
      if (tDs == DEVICE_DISCONNECTED_C || isnan(tNtc)) {
        Serial.println("# Cannot store a point: a sensor reading is invalid");
      } else if (c == 'c' || c == 'C') {
        refCold = tDs; ntcCold = tNtc;
        Serial.print("# COLD point: DS18B20 = "); Serial.print(refCold, 2);
        Serial.print("  NTC = "); Serial.println(ntcCold, 2);
        computeCalibration();
      } else {
        refHot = tDs; ntcHot = tNtc;
        Serial.print("# HOT point: DS18B20 = "); Serial.print(refHot, 2);
        Serial.print("  NTC = "); Serial.println(ntcHot, 2);
        computeCalibration();
      }
    }
  }
}

int averageRead(int pin) {
  long sum = 0;
  for (int i = 0; i < 16; i++) sum += analogRead(pin);
  return sum / 16;
}

void setup() {
  Serial.begin(115200);
  pinMode(KY028_DO, INPUT);
  ds.begin();
  ds.setResolution(12);          // 0.0625 C steps, 750 ms per conversion
  dht.begin();

  // TASK: load a saved calibration, if there is one
  CalData d;
  EEPROM.get(EE_ADDR, d);
  if (d.magic == EE_MAGIC && !isnan(d.gain) && !isnan(d.offset)) {
    gain = d.gain; offset = d.offset;
  }
  Serial.print("# NTC calibration: gain = "); Serial.print(gain, 4);
  Serial.print("  offset = "); Serial.println(offset, 3);
  Serial.println("# Commands: c = cold point, h = hot point, x = clear");
}

void loop() {
  ds.requestTemperatures();      // blocks ~750 ms at 12-bit
  float tDs   = ds.getTempCByIndex(0);
  float tDht  = dht.readTemperature();
  float hDht  = dht.readHumidity();
  float tNtc  = ntcCelsius(averageRead(NTC_PIN), NTC_ON_GROUND_SIDE);
  // KY-028 analog side uses the same kind of NTC, but the divider is upside down
  float tK028 = ntcCelsius(averageRead(KY028_AO), !NTC_ON_GROUND_SIDE);
  float corrected = gain * tNtc + offset;          // TASK: one line corrects every reading

  handleSerial(tDs, tNtc);                         // TASK

  Serial.print("DS18B20:"); Serial.print(tDs, 2);
  Serial.print(" DHT11:");  Serial.print(tDht, 1);
  Serial.print(" KY013:");  Serial.print(tNtc, 2);
  Serial.print(" KY013_cal:"); Serial.print(corrected, 2);   // TASK
  Serial.print(" KY028:");  Serial.print(tK028, 2);
  Serial.print(" KY028_alarm:"); Serial.print(digitalRead(KY028_DO) * 5 + 20);  // 20 or 25 on plot
  Serial.print(" Humidity:"); Serial.println(hDht, 0);

  if (tDs == DEVICE_DISCONNECTED_C) Serial.println("# DS18B20 not found - check wiring");
  if (isnan(tDht)) Serial.println("# DHT11 read failed");

  delay(1250);                   // DHT11 needs at least 1 s between reads
}
