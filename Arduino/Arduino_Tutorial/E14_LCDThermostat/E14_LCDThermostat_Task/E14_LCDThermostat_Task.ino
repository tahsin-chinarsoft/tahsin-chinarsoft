// ChinarSoft - Arduino Sensor Kit, Episode 14 - TASK SOLUTION
// Task: add an over-temperature alarm - at setpoint + 5 C, blink the screen and force the relay off.
//
// What changed vs E14_LCDThermostat.ino:
//   - New checkAlarm(): overTemp = tempC > setpoint + 5. While it is true the relay is forced OFF
//     (even in manual override) and the LCD blinks twice a second (500 ms on / 500 ms off).
//   - checkAlarm() runs right AFTER control(), so the alarm always wins over the setpoint logic,
//     and once per loop() pass so the blink keeps going between temperature readings.
//   - The main page shows "HOT!" instead of "HEAT/off" while the alarm is active.
//   Everything else (menu, hysteresis, buttons, fail-safe on sensor error) is unchanged.
//
// Wiring: identical to Episode 14, no extra parts.
//   The shield plugs on top of the Uno and uses: D4-D9 (LCD), D10 (backlight), A0 (buttons)
//   DS18B20 (KY-001) S -> D2 | middle -> 5V | - -> GND
//   KY-019 relay     S -> D3 | + -> 5V | - -> GND   (12 V heater pad / lamp only)
//
// Buttons: UP/DOWN change value, LEFT/RIGHT change page, SELECT = manual override on/off
// Libraries: LiquidCrystal (built in), OneWire, DallasTemperature

#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);   // RS, EN, D4, D5, D6, D7
const int BUTTONS_PIN = A0;
const int DS_PIN = 2;
const int RELAY_PIN = 3;
const float ALARM_MARGIN = 5.0;        // TASK: alarm at setpoint + 5 C

OneWire oneWire(DS_PIN);
DallasTemperature ds(&oneWire);

enum Button { BTN_NONE, BTN_RIGHT, BTN_UP, BTN_DOWN, BTN_LEFT, BTN_SELECT };
enum Page { PAGE_MAIN, PAGE_SETPOINT, PAGE_HYST, PAGE_COUNT };

float setpoint = 30.0;      // C
float hysteresis = 1.0;     // C, heater turns on below (setpoint - hyst), off above setpoint
float tempC = NAN;
bool heaterOn = false;
bool manualOverride = false;
bool overTemp = false;      // TASK: alarm state
bool screenOn = true;       // TASK: remembers what we last sent to the LCD (avoid sending it every pass)
int page = PAGE_MAIN;
unsigned long lastControl = 0;

byte degreeChar[8] = {0b01100, 0b10010, 0b10010, 0b01100, 0, 0, 0, 0};

// All 5 buttons share A0 through a resistor ladder. Thresholds are midpoints
// between typical readings (0, ~100, ~255, ~410, ~640, 1023). Print analogRead(A0)
// once if your shield differs.
Button readButton() {
  int v = analogRead(BUTTONS_PIN);
  if (v < 60)  return BTN_RIGHT;
  if (v < 200) return BTN_UP;
  if (v < 350) return BTN_DOWN;
  if (v < 520) return BTN_LEFT;
  if (v < 850) return BTN_SELECT;
  return BTN_NONE;
}

Button buttonPressed() {          // returns a button once per press (with debounce)
  static Button last = BTN_NONE;
  static unsigned long t = 0;
  Button b = readButton();
  if (b != last && millis() - t > 50) {
    t = millis();
    last = b;
    return b;
  }
  return BTN_NONE;
}

void control() {
  if (isnan(tempC)) { heaterOn = false; }                       // sensor fault = heater OFF (fail-safe)
  else if (manualOverride) { heaterOn = true; }
  else if (tempC < setpoint - hysteresis) { heaterOn = true; }
  else if (tempC > setpoint) { heaterOn = false; }
  // between the two limits: keep the current state (that's the hysteresis)
  digitalWrite(RELAY_PIN, heaterOn ? HIGH : LOW);
}

// TASK: show or hide the LCD, but only talk to it when the state changes
void setScreen(bool on) {
  if (on == screenOn) return;
  screenOn = on;
  if (on) lcd.display(); else lcd.noDisplay();
}

// TASK: over-temperature alarm. Call it AFTER control(), so it always wins.
// A NAN reading (sensor error) compares false -> no alarm, control() already switched the heater off.
void checkAlarm() {
  overTemp = tempC > setpoint + ALARM_MARGIN;
  if (overTemp) {
    heaterOn = false;
    digitalWrite(RELAY_PIN, LOW);                  // force the relay off, even in manual override
    if ((millis() / 500) % 2) setScreen(false);    // blink: 500 ms off ...
    else setScreen(true);                          // ... 500 ms on
  } else {
    setScreen(true);                               // alarm gone: screen stays on
  }
}

void draw() {
  lcd.setCursor(0, 0);
  switch (page) {
    case PAGE_MAIN:
      lcd.print("T:");
      if (isnan(tempC)) lcd.print(" ERR ");
      else { lcd.print(tempC, 1); lcd.write(byte(0)); lcd.print("C"); }
      lcd.print("        ");
      lcd.setCursor(0, 1);
      lcd.print("Set:"); lcd.print(setpoint, 1);
      if (overTemp) lcd.print(" HOT!");            // TASK: alarm text
      else lcd.print(heaterOn ? " HEAT" : "  off");
      lcd.print(manualOverride ? "M" : " ");
      break;
    case PAGE_SETPOINT:
      lcd.print("Setpoint        ");
      lcd.setCursor(0, 1);
      lcd.print("< "); lcd.print(setpoint, 1); lcd.write(byte(0)); lcd.print("C >      ");
      break;
    case PAGE_HYST:
      lcd.print("Hysteresis      ");
      lcd.setCursor(0, 1);
      lcd.print("< "); lcd.print(hysteresis, 1); lcd.write(byte(0)); lcd.print("C >      ");
      break;
  }
}

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  lcd.begin(16, 2);
  lcd.createChar(0, degreeChar);
  lcd.print("ChinarSoft");
  lcd.setCursor(0, 1);
  lcd.print("Thermostat v1");
  ds.begin();
  ds.setWaitForConversion(false);    // don't block 750 ms: buttons stay responsive
  ds.requestTemperatures();
  delay(1000);
  lcd.clear();
}

void loop() {
  // Temperature: start a conversion, collect it 800 ms later
  if (millis() - lastControl > 800) {
    lastControl = millis();
    float t = ds.getTempCByIndex(0);
    tempC = (t == DEVICE_DISCONNECTED_C) ? NAN : t;
    ds.requestTemperatures();
    control();
    checkAlarm();                    // TASK: after control(), so the alarm wins
    draw();
  }

  checkAlarm();                      // TASK: every pass, keeps the blink running at 2 Hz

  Button b = buttonPressed();
  if (b == BTN_NONE) return;

  if (b == BTN_RIGHT) page = (page + 1) % PAGE_COUNT;
  if (b == BTN_LEFT)  page = (page + PAGE_COUNT - 1) % PAGE_COUNT;
  if (b == BTN_SELECT) manualOverride = !manualOverride;
  if (b == BTN_UP || b == BTN_DOWN) {
    float dir = (b == BTN_UP) ? 1 : -1;
    if (page == PAGE_SETPOINT || page == PAGE_MAIN) setpoint = constrain(setpoint + dir * 0.5, 5, 80);
    if (page == PAGE_HYST) hysteresis = constrain(hysteresis + dir * 0.1, 0.1, 5);
  }
  control();
  checkAlarm();                      // TASK: a button press must not switch the heater back on
  draw();
}
