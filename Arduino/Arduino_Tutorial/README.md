# Arduino Sensor Kit Tutorial · ChinarSoft

Code for the **ChinarSoft Arduino Sensor Kit** video series: a 37-module sensor kit plus 11 extra boards, from your very first Blink to a WiFi dashboard. Every episode explains how a module works, shows where it fails, and builds a small project with it.

Each folder holds one sketch (Arduino needs the folder and the `.ino` file to share a name) and a short README with the libraries and the wiring.

## Episodes

| # | Episode | Topic | Extra libraries | Video |
| --- | --- | --- | --- | --- |
| 0 | [Arduino Basics](E00_Blink/) | the board, digital vs analog, the IDE and your first Blink | — | coming soon |
| 1 | [Smart Doorbell](E01_Doorbell/) | button + active vs passive buzzer | — | coming soon |
| 2 | [Mood Lamp](E02_MoodLamp/) | RGB, two-colour and 7-colour LED modules | — | coming soon |
| 3 | [Secret Knock Lock](E03_SecretKnock/) | tilt, vibration and tap switches | — | coming soon |
| 4 | [Magnet Lab](E04_MagnetLab/) | reed switches and Hall sensors | — | coming soon |
| 5 | [Laser Tripwire](E05_LaserTripwire/) | laser, photoresistor, light barrier, magic cup | — | coming soon |
| 6 | [Temperature Shoot-out](E06_TempShootout/) | NTC vs DS18B20 vs DHT11 | OneWire, DallasTemperature, DHT_sensor_library + Adafruit_Unified_Sensor | coming soon |
| 7 | [Clap Switch](E07_ClapSwitch/) | KY-037 vs KY-038 microphones | — | coming soon |
| 8 | [IR Inspection Station](E08_IRStation/) | obstacle, line and flame sensors | — | coming soon |
| 9 | [IR Remote Cloner](E09_IRCloner/) | IR receiver, IR LED, HX1838 remote | IRremote | coming soon |
| 10 | [Machine Menu](E10_MenuController/) | rotary encoder, joystick, touch sensor | — | coming soon |
| 11 | [Pulse Sensor](E11_PulseSensor/) | KY-039 heartbeat sensor | — | coming soon |
| 12 | [Smart Lamp](E12_SmartLamp/) | relay module + clap, light and door sensors | — | coming soon |
| 13 | [Keypad Lock](E13_KeypadLock/) | 4x3 keypad, EEPROM PIN, relay | Keypad | coming soon |
| 14 | [LCD Thermostat](E14_LCDThermostat/) | LCD keypad shield + DS18B20 + relay | OneWire, DallasTemperature | coming soon |
| 15 | [Reaction Game](E15_ReactionGame/) | joystick shield | — | coming soon |
| 16 | [Data Logger](E16_DataLogger/) | SD card module, CSV logging | OneWire, DallasTemperature, DHT_sensor_library + Adafruit_Unified_Sensor | coming soon |
| 17 | [Gas Sensors](E17_GasSensors/) | MQ-3, MQ-4, MQ-7, MQ-135 | — | coming soon |
| 18 | [Parking Assistant](E18_ParkingAssistant/) | HC-SR04 with temperature correction | OneWire, DallasTemperature | coming soon |
| 19 | [WiFi Motors](E19_WiFiMotors/) | NodeMCU motor shield, phone control | — | coming soon |
| 20 | [WiFi Dashboard](E20_WiFiDashboard/) | NodeMCU live sensor dashboard | OneWire, DallasTemperature | coming soon |

Episodes 0–18 run on an **Arduino Uno**. Episodes 19–20 run on a **NodeMCU 1.0 (ESP-12E)**.

## Installing the libraries

Many sketches need nothing but the Arduino core. The others need the libraries in [`libraries/`](libraries). Install them in one of two ways.

**Option 1: Library Manager (recommended).** In the Arduino IDE open *Tools → Manage Libraries…* and install:

| Library | Search for | Tested version |
| --- | --- | --- |
| IRremote | `IRremote` (by Armin Joachimsmeyer) | 4.7.1 |
| DHT sensor library | `DHT sensor library` (by Adafruit) | 1.4.7 |
| Adafruit Unified Sensor | `Adafruit Unified Sensor` | 1.1.15 |
| OneWire | `OneWire` | 2.3.8 |
| DallasTemperature | `DallasTemperature` | 4.0.6 |
| Keypad | `Keypad` (by Mark Stanley, Alexander Brevig) | 3.1.1 |

LiquidCrystal, SD, SPI and EEPROM come with the IDE.

**Option 2: copy the folders.** Copy each folder from [`libraries/`](libraries) into your sketchbook's `libraries` folder (usually `Documents/Arduino/libraries`) and restart the IDE. These are the exact versions the sketches were tested with.

**ESP8266 (Episodes 19–20).** In *File → Preferences → Additional boards manager URLs* add
`https://arduino.esp8266.com/stable/package_esp8266com_index.json`, then install **esp8266 by ESP8266 Community** in *Tools → Board → Boards Manager* and select **NodeMCU 1.0 (ESP-12E Module)**.

## Safety

- Episode 12–14: switch **12 V loads only** with the relay. Never mains on a breadboard.
- Episode 17: MQ gas sensors are learning parts, **not certified detectors**. No open flames.
- Episode 5: never point the laser at eyes. Episode 8: lighter or candle at arm's length.
- Episode 19: never power motors from USB.

## Licences

The sketches are free to use for learning and your own projects. Each folder in `libraries/` keeps its own licence from its original authors.

---

[chinarsoft.com](https://www.chinarsoft.com)
