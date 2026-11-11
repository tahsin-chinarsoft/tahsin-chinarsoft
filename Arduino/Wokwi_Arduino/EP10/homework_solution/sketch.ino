// Arduino in Your Browser — Episode 10 HOMEWORK SOLUTION: Celsius / Fahrenheit button
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// DHT22 -> pin 2, LCD1602 I2C -> A4/A5 (0x27), button pin 3 -> GND (INPUT_PULLUP).
// Each press switches between C and F. Debounced (Episode 4), display updates at once.

#include <DHT.h>
#include <LiquidCrystal_I2C.h>

DHT dht(2, DHT22);
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int BUTTON = 3;
bool fahrenheit = false;
bool lastButton = HIGH;
unsigned long lastChange = 0;
unsigned long lastRead = 0;

void setup() {
  pinMode(BUTTON, INPUT_PULLUP);
  dht.begin();
  lcd.init();
  lcd.backlight();
}

void showLine(int row, const char* label, float value, const char* unit) {
  char num[8];
  dtostrf(value, 5, 1, num);
  lcd.setCursor(0, row);
  lcd.print(label);
  lcd.print(num);
  lcd.print(unit);
}

void loop() {
  bool now = digitalRead(BUTTON);
  if (now != lastButton && millis() - lastChange > 50) {   // debounce
    lastChange = millis();
    lastButton = now;
    if (now == LOW) {                                      // pressed
      fahrenheit = !fahrenheit;
      lastRead = millis() - 2000;                          // update right away
    }
  }

  if (millis() - lastRead < 2000) return;
  lastRead = millis();

  float t = dht.readTemperature(fahrenheit);              // true = Fahrenheit
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) {
    lcd.setCursor(0, 0);
    lcd.print("Sensor error!   ");
    return;
  }
  showLine(0, "Temp: ", t, fahrenheit ? " \337F  " : " \337C  ");
  showLine(1, "Hum:  ", h, " %   ");
}
