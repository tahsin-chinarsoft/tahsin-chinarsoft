# Episode 13 · Keypad Lock
_4x3 keypad, EEPROM PIN, relay_

**Video:** coming soon

**Board:** Arduino Uno  
**Sketch:** `E13_KeypadLock.ino`

## Libraries

- Keypad
- EEPROM (built in)

Library folders are in [`../libraries`](../libraries). See the [main README](../README.md#installing-the-libraries) for how to install them.

## Notes and wiring (from the sketch header)

```text
ChinarSoft - Arduino Sensor Kit, Episode 13
4x3 membrane keypad + relay (KY-019) + passive buzzer + two-color LED
Project: Password door lock with EEPROM-saved PIN and brute-force lockout

Keys:  digits = enter PIN   # = confirm   * = clear
       To change the PIN: unlock first, then type  *0#  , then the new PIN (4-8 digits) and #.

Wiring (Arduino Uno), keypad pins from left to right with keys facing you:
  Keypad pin 1..7  ->  D9 D8 D7 D6 D5 D4 D3    (check yours: rows R1-R4, cols C1-C3)
  KY-019 relay  S  -> A0    (drives a 12 V solenoid lock / LED strip)
  Passive buzzer S -> A1
  Two-color LED R -> A2, G -> A3 (220 ohm)

Library: Keypad (Mark Stanley, Alexander Brevig)
```
