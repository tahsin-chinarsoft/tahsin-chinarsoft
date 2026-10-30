# Episode 1 · Smart Doorbell
_Button + active vs passive buzzer_

**Video:** coming soon

**Board:** Arduino Uno  
**Sketch:** `E01_Doorbell.ino`

## Libraries

- None, only the Arduino core.

Library folders are in [`../libraries`](../libraries). See the [main README](../README.md#installing-the-libraries) for how to install them.

## Notes and wiring (from the sketch header)

```text
ChinarSoft - Arduino Sensor Kit, Episode 1
Button (KY-004) + Active Buzzer (KY-012) + Passive Buzzer (KY-006): Smart Doorbell

Wiring (Arduino Uno):
  Button  S -> D2   | middle -> 5V | - -> GND
  Active buzzer  S -> D8 | - -> GND
  Passive buzzer S -> D9 | - -> GND

Short press : "ding-dong" on the passive buzzer
Long press  : (> 1 s) urgent beeps on the active buzzer
No library needed.
```

## Homework solution

The "Your task" from the end of this episode, solved in full: [`E01_Doorbell_Task/E01_Doorbell_Task.ino`](E01_Doorbell_Task/E01_Doorbell_Task.ino)

Same wiring as the main sketch unless the file header says otherwise.
