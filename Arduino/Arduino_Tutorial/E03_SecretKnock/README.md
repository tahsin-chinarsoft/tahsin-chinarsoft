# Episode 3 · Secret Knock Lock
_Tilt, vibration and tap switches_

**Video:** coming soon

**Board:** Arduino Uno  
**Sketch:** `E03_SecretKnock.ino`

## Libraries

- None, only the Arduino core.

Library folders are in [`../libraries`](../libraries). See the [main README](../README.md#installing-the-libraries) for how to install them.

## Notes and wiring (from the sketch header)

```text
ChinarSoft - Arduino Sensor Kit, Episode 3
Tap (KY-031), Vibration switch (KY-002), Tilt switches (KY-020 / KY-017 / other)
Project: Secret-knock lock with a tamper (tilt) alarm

Wiring (Arduino Uno):
  Tap sensor KY-031     S -> D2   | middle -> 5V | - -> GND
  Vibration KY-002      S -> D3   | middle -> 5V | - -> GND
  Tilt switch A         S -> D4   | middle -> 5V | - -> GND
  Tilt switch B         S -> D7
  Tilt switch C         S -> D8
  Two-color LED         R -> D5, G -> D6 (220 ohm each), - -> GND
  Active buzzer         S -> D12, - -> GND

Serial: 'L' = learn a new knock pattern, 'T' = print tilt switch states
```

## Homework solution

The "Your task" from the end of this episode, solved in full: [`E03_SecretKnock_Task/E03_SecretKnock_Task.ino`](E03_SecretKnock_Task/E03_SecretKnock_Task.ino)

Same wiring as the main sketch unless the file header says otherwise.
