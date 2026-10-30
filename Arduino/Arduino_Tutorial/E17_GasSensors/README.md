# Episode 17 · Gas Sensors
_MQ-3, MQ-4, MQ-7, MQ-135_

**Video:** coming soon

**Board:** Arduino Uno  
**Sketch:** `E17_GasSensors.ino`

## Libraries

- EEPROM (built in)

Library folders are in [`../libraries`](../libraries). See the [main README](../README.md#installing-the-libraries) for how to install them.

## Notes and wiring (from the sketch header)

```text
ChinarSoft - Arduino Sensor Kit, Episode 17
MQ gas sensors: MQ-3 (alcohol), MQ-4 (methane/CNG), MQ-7 (carbon monoxide), MQ-135 (air quality)
Project: 4-channel gas monitor with warm-up, clean-air calibration and relative alarms

!!! These are hobby sensors. NOT a certified gas/CO detector. Never rely on them for safety. !!!

POWER: each heater draws ~150 mA. Four sensors = ~600 mA -> more than USB can give.
       Power the sensors from a separate 5 V / 2 A supply and connect its GND to Arduino GND.

Wiring (Arduino Uno):
  MQ-3   AO -> A0     MQ-4   AO -> A1
  MQ-7   AO -> A2     MQ-135 AO -> A3
  All VCC -> external 5 V, all GND -> external GND + Arduino GND
  Active buzzer S -> D12

Serial (115200): 'c' = calibrate in clean air (saves R0 to EEPROM)
Output: ratio Rs/R0 per sensor. 1.00 = clean air, lower = more gas.
```

## Homework solution

The "Your task" from the end of this episode, solved in full: [`E17_GasSensors_Task/E17_GasSensors_Task.ino`](E17_GasSensors_Task/E17_GasSensors_Task.ino)

Same wiring as the main sketch unless the file header says otherwise.
