# Episode 6 · Temperature Shoot-out
_NTC vs DS18B20 vs DHT11_

**Video:** coming soon

**Board:** Arduino Uno  
**Sketch:** `E06_TempShootout.ino`

## Libraries

- OneWire
- DallasTemperature
- DHT_sensor_library + Adafruit_Unified_Sensor

Library folders are in [`../libraries`](../libraries). See the [main README](../README.md#installing-the-libraries) for how to install them.

## Notes and wiring (from the sketch header)

```text
ChinarSoft - Arduino Sensor Kit, Episode 6
Temperature shoot-out: KY-013 analog NTC, KY-028 digital NTC, KY-001 DS18B20, KY-015 DHT11

Wiring (Arduino Uno):
  KY-013 analog temp    S -> A0 | middle -> 5V | - -> GND
  KY-028 digital temp   AO -> A1 | DO -> D3 | + -> 5V | G -> GND
  KY-001 DS18B20        S -> D4 | middle -> 5V | - -> GND   (board has a 4.7k pull-up)
  KY-015 DHT11          S -> D5 | middle -> 5V | - -> GND

Libraries: OneWire, DallasTemperature, DHT sensor library (Adafruit) + Adafruit Unified Sensor
Output: Serial Plotter at 115200 baud
```

## Homework solution

The "Your task" from the end of this episode, solved in full: [`E06_TempShootout_Task/E06_TempShootout_Task.ino`](E06_TempShootout_Task/E06_TempShootout_Task.ino)

Same wiring as the main sketch unless the file header says otherwise.
