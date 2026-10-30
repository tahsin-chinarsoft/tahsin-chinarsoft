# Episode 16 · Data Logger
_SD card module, CSV logging_

**Video:** coming soon

**Board:** Arduino Uno  
**Sketch:** `E16_DataLogger.ino`

## Libraries

- SPI (built in)
- SD (built in)
- OneWire
- DallasTemperature
- DHT_sensor_library + Adafruit_Unified_Sensor

Library folders are in [`../libraries`](../libraries). See the [main README](../README.md#installing-the-libraries) for how to install them.

## Notes and wiring (from the sketch header)

```text
ChinarSoft - Arduino Sensor Kit, Episode 16
SD card module + DS18B20 + DHT11 + photoresistor
Project: Environmental data logger -> CSV file you can open in Excel

Wiring (Arduino Uno):
  SD module   CS -> D10 | MOSI -> D11 | MISO -> D12 | SCK -> D13 | VCC -> 5V | GND -> GND
  DS18B20     S -> D4
  DHT11       S -> D5
  KY-018 LDR  S -> A0
  KY-004 button S -> D2  (press = close the file safely before removing the card)
  Status LED  D3 (220 ohm): blinks on every write, fast blink = error

Card: FAT16/FAT32, max 32 GB, file names 8.3 format (LOG007.CSV)
Libraries: SD (built in), OneWire, DallasTemperature, DHT sensor library
```

## Homework solution

The "Your task" from the end of this episode, solved in full: [`E16_DataLogger_Task/E16_DataLogger_Task.ino`](E16_DataLogger_Task/E16_DataLogger_Task.ino)

Same wiring as the main sketch unless the file header says otherwise.
