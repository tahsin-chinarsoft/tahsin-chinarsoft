# Episode 12 · Smart Lamp
_Relay module + clap, light and door sensors_

**Video:** coming soon

**Board:** Arduino Uno  
**Sketch:** `E12_SmartLamp.ino`

## Libraries

- None, only the Arduino core.

Library folders are in [`../libraries`](../libraries). See the [main README](../README.md#installing-the-libraries) for how to install them.

## Notes and wiring (from the sketch header)

```text
ChinarSoft - Arduino Sensor Kit, Episode 12 (Season 1 finale)
Relay module (KY-019) + clap microphone + photoresistor + button + reed switch
Project: Smart lamp
  - Double clap or button press toggles the lamp
  - Door opens (reed) while the room is dark -> lamp on automatically
  - Lamp turns itself off after AUTO_OFF_MS
  - Relay protection: never switch faster than MIN_SWITCH_MS

LOW-VOLTAGE LOAD ONLY ON CAMERA (e.g. 12 V LED strip with its own adapter).

Wiring (Arduino Uno):
  KY-019 relay      S -> D7 | + -> 5V | - -> GND | COM + NO in series with the 12 V lamp
  KY-037 mic        AO -> A0
  KY-018 LDR        S -> A1
  KY-004 button     S -> D2
  KY-021 reed       S -> D4 (magnet on the door, door CLOSED at boot)
```
