# Episode 20 · WiFi Dashboard
_NodeMCU live sensor dashboard_

**Video:** coming soon

**Board:** NodeMCU 1.0 (ESP-12E Module)  
**Sketch:** `E20_WiFiDashboard.ino`

## Libraries

- ESP8266 board package
- OneWire
- DallasTemperature

Library folders are in [`../libraries`](../libraries). See the [main README](../README.md#installing-the-libraries) for how to install them.

## Notes and wiring (from the sketch header)

```text
ChinarSoft - Arduino Sensor Kit, Episode 20 (Series finale)
NodeMCU (ESP8266) WiFi sensor dashboard
  - DS18B20 temperature, MQ-135 air quality, reed-switch door state
  - Joins your home WiFi; open http://chinarsoft.local (or the IP from Serial Monitor)
  - Live page with values + 10-minute temperature chart, JSON API at /data
  - If home WiFi is not available, it starts its own network "ChinarSoft-Dashboard"

Board: "NodeMCU 1.0 (ESP-12E Module)"
Wiring:
  DS18B20 (KY-001)  S -> D5 (GPIO14) | middle -> 3V3 | - -> GND
  MQ-135 AO -> 10k -> A0, and A0 -> 20k -> GND   (divider: MQ outputs up to 5 V, NodeMCU A0 max 3.3 V)
           MQ-135 VCC -> 5 V supply (VIN/USB 5V), GND -> GND
  KY-021 reed       S -> D6 (GPIO12) | - -> GND   (internal pull-up used)
Libraries: OneWire, DallasTemperature (+ ESP8266 core libraries)
```
