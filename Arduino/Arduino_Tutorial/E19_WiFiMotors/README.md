# Episode 19 · WiFi Motors
_NodeMCU motor shield, phone control_

**Video:** coming soon

**Board:** NodeMCU 1.0 (ESP-12E Module)  
**Sketch:** `E19_WiFiMotors.ino`

## Libraries

- ESP8266 board package

Library folders are in [`../libraries`](../libraries). See the [main README](../README.md#installing-the-libraries) for how to install them.

## Notes and wiring (from the sketch header)

```text
ChinarSoft - Arduino Sensor Kit, Episode 19
NodeMCU (ESP-12E / ESP8266) + NodeMCU Motor Shield (L293D)
Project: Control two DC motors from your phone over WiFi - no app, just a web page
  - The NodeMCU creates its own WiFi network ("ChinarSoft-Motors")
  - Open http://192.168.4.1 on the phone: forward / back / left / right / stop + speed slider
  - SAFETY: if the page stops sending commands for 1 s (WiFi lost), the motors stop

Board: "NodeMCU 1.0 (ESP-12E Module)"  (Boards Manager: esp8266 by ESP8266 Community)
Shield pins (fixed by the shield):
  Motor A: speed D1 (GPIO5), direction D3 (GPIO0)
  Motor B: speed D2 (GPIO4), direction D4 (GPIO2)
Power: motors from VM terminal (4.5-9 V battery), keep the VIN/VM jumper as your shield's manual says.
       Never power motors from the USB port.
```
