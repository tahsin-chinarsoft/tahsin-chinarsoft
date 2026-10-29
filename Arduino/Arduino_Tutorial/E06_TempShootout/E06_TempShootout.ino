// ChinarSoft - Arduino Sensor Kit, Episode 6
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

float ntcCelsius(int raw, bool ntcOnGroundSide) {
  if (raw <= 0 || raw >= 1023) return NAN;
  float r;
  if (ntcOnGroundSide) r = R_FIXED * raw / (1023.0 - raw);
  else                 r = R_FIXED * (1023.0 - raw) / raw;
  // Beta form of the Steinhart-Hart equation
  float invT = 1.0 / (T_NOMINAL + 273.15) + log(r / R_NOMINAL) / B_COEFF;
  return 1.0 / invT - 273.15;
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
}

void loop() {
  ds.requestTemperatures();      // blocks ~750 ms at 12-bit
  float tDs   = ds.getTempCByIndex(0);
  float tDht  = dht.readTemperature();
  float hDht  = dht.readHumidity();
  float tNtc  = ntcCelsius(averageRead(NTC_PIN), NTC_ON_GROUND_SIDE);
  // KY-028 analog side uses the same kind of NTC, but the divider is upside down
  float tK028 = ntcCelsius(averageRead(KY028_AO), !NTC_ON_GROUND_SIDE);

  Serial.print("DS18B20:"); Serial.print(tDs, 2);
  Serial.print(" DHT11:");  Serial.print(tDht, 1);
  Serial.print(" KY013:");  Serial.print(tNtc, 2);
  Serial.print(" KY028:");  Serial.print(tK028, 2);
  Serial.print(" KY028_alarm:"); Serial.print(digitalRead(KY028_DO) * 5 + 20);  // 20 or 25 on plot
  Serial.print(" Humidity:"); Serial.println(hDht, 0);

  if (tDs == DEVICE_DISCONNECTED_C) Serial.println("# DS18B20 not found - check wiring");
  if (isnan(tDht)) Serial.println("# DHT11 read failed");

  delay(1250);                   // DHT11 needs at least 1 s between reads
}
