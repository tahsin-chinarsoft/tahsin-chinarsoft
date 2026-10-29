# Episode 18 · Parking Assistant
_HC-SR04 with temperature correction_

**Video:** coming soon

**Board:** Arduino Uno  
**Sketch:** `E18_ParkingAssistant.ino`

## Libraries

- OneWire
- DallasTemperature

Library folders are in [`../libraries`](../libraries). See the [main README](../README.md#installing-the-libraries) for how to install them.

## Notes and wiring (from the sketch header)

```text
ChinarSoft - Arduino Sensor Kit, Episode 18
HC-SR04 ultrasonic + passive buzzer + RGB LED + DS18B20 (speed-of-sound correction)
Project: Garage parking assistant
  > 100 cm green, 100..40 cm yellow + slow beeps, < 40 cm red + fast beeps, < 15 cm STOP (solid tone)
  Beeps go quiet after the car has been parked (distance stable) for 10 s

Wiring (Arduino Uno):
  HC-SR04   VCC -> 5V | TRIG -> D7 | ECHO -> D10 | GND -> GND
  Passive buzzer S -> D8
  RGB module R -> D9, G -> D5, B -> D6 (PWM pins, not D3/D11), - -> GND
  DS18B20 (KY-001) S -> D4
Libraries: OneWire, DallasTemperature
```
