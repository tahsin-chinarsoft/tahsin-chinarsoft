# Episode 15 · Reaction Game
_Joystick shield_

**Video:** coming soon

**Board:** Arduino Uno  
**Sketch:** `E15_ReactionGame.ino`

## Libraries

- EEPROM (built in)

Library folders are in [`../libraries`](../libraries). See the [main README](../README.md#installing-the-libraries) for how to install them.

## Notes and wiring (from the sketch header)

```text
ChinarSoft - Arduino Sensor Kit, Episode 15
Joystick Shield (Funduino style): buttons A-F + joystick with push button
Project: Reaction-time game, 5 rounds, best score saved in EEPROM
  At GO the Serial Monitor shows a target: a button letter (A/B/C/D) or a joystick
  direction (UP/DOWN/LEFT/RIGHT). Hit it as fast as you can.
  Pressing before "GO" = false start (+500 ms penalty).

Shield pinout (fixed, typical):
  A -> D2  B -> D3  C -> D4  D -> D5  E -> D6  F -> D7  Joystick push (K) -> D8
  X -> A0  Y -> A1
  Set the shield's voltage switch to 5V for the Uno.
On-board LED D13 lights at "GO".
```
