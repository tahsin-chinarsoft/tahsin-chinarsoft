# Episode 5 · Laser Tripwire
_Laser, photoresistor, light barrier, magic cup_

**Video:** coming soon

**Board:** Arduino Uno  
**Sketch:** `E05_LaserTripwire.ino`

## Libraries

- None, only the Arduino core.

Library folders are in [`../libraries`](../libraries). See the [main README](../README.md#installing-the-libraries) for how to install them.

## Notes and wiring (from the sketch header)

```text
ChinarSoft - Arduino Sensor Kit, Episode 5
Laser (KY-008), Photoresistor (KY-018), Light interrupter (KY-010), Magic cup (KY-027)
Project: Laser tripwire alarm
  - Laser shines on the photoresistor across a doorway
  - Swipe a card through the KY-010 slot to arm / disarm
  - Magic cup LED shows the state, tilt the cup to silence the alarm

Wiring (Arduino Uno):
  KY-008 laser     S -> D7  | - -> GND          (middle pin not connected on most boards)
  KY-018 LDR       S -> A0  | middle -> 5V | - -> GND
  KY-010 slot      S -> D2  | middle -> 5V | - -> GND
  KY-027 magic cup L -> D9 (PWM LED) | S -> D4 (tilt) | + -> 5V | G -> GND
  Active buzzer    S -> D12 | - -> GND
```
