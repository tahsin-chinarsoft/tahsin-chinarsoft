# Episode 2 · Mood Lamp
_RGB, two-colour and 7-colour LED modules_

**Video:** coming soon

**Board:** Arduino Uno  
**Sketch:** `E02_MoodLamp.ino`

## Libraries

- None, only the Arduino core.

Library folders are in [`../libraries`](../libraries). See the [main README](../README.md#installing-the-libraries) for how to install them.

## Notes and wiring (from the sketch header)

```text
ChinarSoft - Arduino Sensor Kit, Episode 2
LED modules: RGB (KY-016 / KY-009), Two-color (KY-011 / KY-029), 7-color flash (KY-034)
Project: Mood lamp with a status light

Wiring (Arduino Uno) - all PWM pins except D7:
  RGB module      R -> D9  | G -> D10 | B -> D11 | - -> GND
  Two-color LED   R -> D5  | G -> D6  | - -> GND
  7-color flash   S -> D7  | - -> GND
  KY-009 and KY-011 have NO resistors on board: add 220 ohm in series on each color!

Serial commands (9600 baud):
  m = mood mode (slow rainbow)    r/g/b/w = fixed color    0 = off
  f = toggle 7-color flash LED
```
