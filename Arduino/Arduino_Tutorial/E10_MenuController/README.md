# Episode 10 · Machine Menu
_Rotary encoder, joystick, touch sensor_

**Video:** coming soon

**Board:** Arduino Uno  
**Sketch:** `E10_MenuController.ino`

## Libraries

- EEPROM (built in)

Library folders are in [`../libraries`](../libraries). See the [main README](../README.md#installing-the-libraries) for how to install them.

## Notes and wiring (from the sketch header)

```text
ChinarSoft - Arduino Sensor Kit, Episode 10
Joystick (KY-023), Rotary encoder (KY-040), Metal touch sensor (KY-036)
Project: Machine settings menu (Serial Monitor)
  Encoder turn / joystick up-down : move through the menu, or change a value while editing
  Encoder push / joystick push    : enter / leave edit mode
  Touch the KY-036 wire           : SAVE all settings to EEPROM (they survive a reset / power cut)

Wiring (Arduino Uno):
  KY-040 encoder   CLK -> D2 | DT -> D3 | SW -> D5 | + -> 5V | GND -> GND
  KY-023 joystick  VRx -> A0 | VRy -> A1 | SW -> D4 | +5V -> 5V | GND -> GND
  KY-036 touch     AO -> A2  | DO -> D6 | + -> 5V | G -> GND
```

## Homework solution

The "Your task" from the end of this episode, solved in full: [`E10_MenuController_Task/E10_MenuController_Task.ino`](E10_MenuController_Task/E10_MenuController_Task.ino)

Same wiring as the main sketch unless the file header says otherwise.
