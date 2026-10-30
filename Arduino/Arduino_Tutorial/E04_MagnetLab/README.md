# Episode 4 · Magnet Lab
_Reed switches and Hall sensors_

**Video:** coming soon

**Board:** Arduino Uno  
**Sketch:** `E04_MagnetLab.ino`

## Libraries

- None, only the Arduino core.

Library folders are in [`../libraries`](../libraries). See the [main README](../README.md#installing-the-libraries) for how to install them.

## Notes and wiring (from the sketch header)

```text
ChinarSoft - Arduino Sensor Kit, Episode 4
Reed switches (KY-025, KY-021) and Hall sensors (KY-003, KY-035, KY-024)
Project: Magnet lab - door sensor, RPM counter, magnet strength & polarity meter

Wiring (Arduino Uno):
  KY-003 Hall switch       S -> D2 (interrupt) | middle -> 5V | - -> GND
  KY-021 mini reed         S -> D4
  KY-025 reed module       DO -> D5 | AO -> (not used) | + -> 5V | G -> GND
  KY-024 linear Hall       DO -> D6 | AO -> A1
  KY-035 analog Hall       S -> A0  | middle -> 5V | - -> GND
  Active buzzer            S -> D12

Serial Plotter friendly: prints "field:<value> door:<0/1> rpm:<value>"
```

## Homework solution

The "Your task" from the end of this episode, solved in full: [`E04_MagnetLab_Task/E04_MagnetLab_Task.ino`](E04_MagnetLab_Task/E04_MagnetLab_Task.ino)

Same wiring as the main sketch unless the file header says otherwise.
