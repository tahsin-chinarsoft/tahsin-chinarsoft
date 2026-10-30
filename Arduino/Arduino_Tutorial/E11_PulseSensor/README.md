# Episode 11 · Pulse Sensor
_KY-039 heartbeat sensor_

**Video:** coming soon

**Board:** Arduino Uno  
**Sketch:** `E11_PulseSensor.ino`

## Libraries

- None, only the Arduino core.

Library folders are in [`../libraries`](../libraries). See the [main README](../README.md#installing-the-libraries) for how to install them.

## Notes and wiring (from the sketch header)

```text
ChinarSoft - Arduino Sensor Kit, Episode 11
Finger detection / heartbeat module (KY-039)
Project: Pulse plotter with BPM estimate (NOT a medical device)

Wiring (Arduino Uno):
  KY-039   S -> A0 | middle -> 5V | - -> GND
  LED      D13 (on-board) blinks on each beat

Use: put a fingertip gently between the IR LED and the sensor, keep still,
     shade it from room light (a piece of black tape or a clothes peg helps).
Serial Plotter at 115200: "pulse:<filtered> bpm:<value>"
```

## Homework solution

The "Your task" from the end of this episode, solved in full: [`E11_PulseSensor_Task/E11_PulseSensor_Task.ino`](E11_PulseSensor_Task/E11_PulseSensor_Task.ino)

Same wiring as the main sketch unless the file header says otherwise.
