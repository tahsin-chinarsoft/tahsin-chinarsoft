# Episode 8 · IR Inspection Station
_Obstacle, line and flame sensors_

**Video:** coming soon

**Board:** Arduino Uno  
**Sketch:** `E08_IRStation.ino`

## Libraries

- None, only the Arduino core.

Library folders are in [`../libraries`](../libraries). See the [main README](../README.md#installing-the-libraries) for how to install them.

## Notes and wiring (from the sketch header)

```text
ChinarSoft - Arduino Sensor Kit, Episode 8
Infrared trio: Obstacle avoidance (KY-032), Line tracking (KY-033), Flame (KY-026)
Project: Mini factory inspection station
  - KY-032 detects a part arriving
  - KY-033 reads a black/white mark on the part -> type A or type B
  - KY-026 is a fire interlock: if it sees a flame, the station stops

Wiring (Arduino Uno):
  KY-032 obstacle   OUT -> D2 | + -> 5V | GND -> GND | EN: leave jumper ON
  KY-033 tracking   S -> D3   | V+ -> 5V | G -> GND
  KY-026 flame      AO -> A0  | DO -> D4 | + -> 5V | G -> GND
  Two-color LED     R -> D5, G -> D6 (220 ohm)
  Active buzzer     S -> D12
```

## Homework solution

The "Your task" from the end of this episode, solved in full: [`E08_IRStation_Task/E08_IRStation_Task.ino`](E08_IRStation_Task/E08_IRStation_Task.ino)

Same wiring as the main sketch unless the file header says otherwise.
