// ChinarSoft - Arduino Sensor Kit, Episode 14
// LCD Keypad Shield (16x2 + 5 buttons on A0) + DS18B20 + relay
// Project: Thermostat with on-screen menu and hysteresis
//
// The shield plugs on top of the Uno and uses: D4-D9 (LCD), D10 (backlight), A0 (buttons)
// Extra wiring on the shield's free headers:
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

OneWire oneWire(DS_PIN);
DallasTemperature ds(&oneWire);

enum Button { BTN_NONE, BTN_RIGHT, BTN_UP, BTN_DOWN, BTN_LEFT, BTN_SELECT };
enum Page { PAGE_MAIN, PAGE_SETPOINT, PAGE_HYST, PAGE_COUNT };

float setpoint = 30.0;      // C
float hysteresis = 1.0;     // C, heater turns on below (setpoint - hyst), off above setpoint
float tempC = NAN;
bool heaterOn = false;
bool manualOverride = false;
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
      lcd.print(heaterOn ? " HEAT" : "  off");
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
    draw();
  }

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
  draw();
}
