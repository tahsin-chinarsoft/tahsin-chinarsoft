# Episode 9 · IR Remote Cloner
_IR receiver, IR LED, HX1838 remote_

**Video:** coming soon

**Board:** Arduino Uno  
**Sketch:** `E09_IRCloner.ino`

## Libraries

- IRremote

Library folders are in [`../libraries`](../libraries). See the [main README](../README.md#installing-the-libraries) for how to install them.

## Notes and wiring (from the sketch header)

```text
ChinarSoft - Arduino Sensor Kit, Episode 9
IR receiver (KY-022), IR transmitter (KY-005), HX1838 remote kit
Project: IR remote cloner + remote-controlled RGB LED
  - Point any remote at the receiver: protocol, address and command are printed
  - Press 'l' in Serial Monitor, then a remote button: that code is LEARNED
  - Press the kit button (or send 's') to TRANSMIT the learned code with KY-005
  - HX1838 remote buttons 1/2/3 control the RGB LED, 0 = off

Wiring (Arduino Uno):
  KY-022 / HX1838 receiver  S -> D2 | middle -> 5V | - -> GND
  KY-005 IR LED             S -> D3 | - -> GND   (D3 = IRremote default send pin on Uno)
  KY-004 button             S -> D4 | middle -> 5V | - -> GND
  RGB module                R -> D5, G -> D6, B -> D9, - -> GND

Library: IRremote (Armin Joachimsmeyer) v4.x
```

## Homework solution

The "Your task" from the end of this episode, solved in full: [`E09_IRCloner_Task/E09_IRCloner_Task.ino`](E09_IRCloner_Task/E09_IRCloner_Task.ino)

Same wiring as the main sketch unless the file header says otherwise.
