# Episode 7 · Clap Switch
_KY-037 vs KY-038 microphones_

**Video:** coming soon

**Board:** Arduino Uno  
**Sketch:** `E07_ClapSwitch.ino`

## Libraries

- None, only the Arduino core.

Library folders are in [`../libraries`](../libraries). See the [main README](../README.md#installing-the-libraries) for how to install them.

## Notes and wiring (from the sketch header)

```text
ChinarSoft - Arduino Sensor Kit, Episode 7
Microphones: KY-037 (big, high sensitivity) and KY-038 (small)
Project: Double-clap switch + sound level meter

Wiring (Arduino Uno):
  KY-037  AO -> A0 | DO -> D2 | + -> 5V | G -> GND
  KY-038  AO -> A1 | DO -> D3 | + -> 5V | G -> GND
  LED (or relay in Episode 12) -> D13 (on-board LED works too)

Serial Plotter at 115200: "big:<p2p> small:<p2p> threshold:<value>"
```
