# Episode 0 · Arduino Basics
_The board, digital vs analog, the IDE and your first Blink_

**Video:** coming soon

**Board:** Arduino Uno  
**Sketch:** `E00_Blink.ino`

## Libraries

- None, only the Arduino core.

Library folders are in [`../libraries`](../libraries). See the [main README](../README.md#installing-the-libraries) for how to install them.

## Notes and wiring (from the sketch header)

```text
ChinarSoft - Arduino Sensor Kit, Episode 0
Arduino basics: the on-board LED (pin 13, "L") - your first program

No wiring needed: the Uno has an LED on pin 13, labelled "L".
Change BLINK_MODE and upload again to try each version:
  0 = classic blink (1 s on, 1 s off)
  1 = SOS in Morse code ( ... --- ... )
  2 = blink without delay() using millis()  (the Arduino stays free to do other work)
Open the Serial Monitor at 9600 baud to see what the board is doing.
```
