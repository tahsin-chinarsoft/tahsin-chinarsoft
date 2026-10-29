# Episode 14 · LCD Thermostat
_LCD keypad shield + DS18B20 + relay_

**Video:** coming soon

**Board:** Arduino Uno  
**Sketch:** `E14_LCDThermostat.ino`

## Libraries

- LiquidCrystal (built in)
- OneWire
- DallasTemperature

Library folders are in [`../libraries`](../libraries). See the [main README](../README.md#installing-the-libraries) for how to install them.

## Notes and wiring (from the sketch header)

```text
ChinarSoft - Arduino Sensor Kit, Episode 14
LCD Keypad Shield (16x2 + 5 buttons on A0) + DS18B20 + relay
Project: Thermostat with on-screen menu and hysteresis

The shield plugs on top of the Uno and uses: D4-D9 (LCD), D10 (backlight), A0 (buttons)
Extra wiring on the shield's free headers:
  DS18B20 (KY-001) S -> D2 | middle -> 5V | - -> GND
  KY-019 relay     S -> D3 | + -> 5V | - -> GND   (12 V heater pad / lamp only)

Buttons: UP/DOWN change value, LEFT/RIGHT change page, SELECT = manual override on/off
Libraries: LiquidCrystal (built in), OneWire, DallasTemperature
```
