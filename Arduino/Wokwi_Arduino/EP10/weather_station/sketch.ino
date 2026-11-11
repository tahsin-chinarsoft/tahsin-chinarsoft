// Arduino in Your Browser — Episode 10: DHT22 weather station on an LCD (done right)
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// DHT22 data -> pin 2. LCD1602 I2C: SDA -> A4, SCL -> A5 (address 0x27).
// Fixed-width numbers (no ghost characters), a reading every 2 s with millis(),
// and an error message if the sensor gives NaN.

#include <DHT.h>
#include <LiquidCrystal_I2C.h>

DHT dht(2, DHT22);
LiquidCrystal_I2C lcd(0x27, 16, 2);
unsigned long lastRead = 0;

void setup() {
  Serial.begin(9600);
  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.print("Weather station");
  delay(1000);
  lcd.clear();
}

// label (6) + number (5) + unit (5) = all 16 characters, every time
void showLine(int row, const char* label, float value, const char* unit) {
  char num[8];
  dtostrf(value, 5, 1, num);          // 9.5 -> "  9.5", 10.0 -> " 10.0"
  lcd.setCursor(0, row);
  lcd.print(label);
  lcd.print(num);
  lcd.print(unit);
}

void loop() {
  if (millis() - lastRead < 2000) return;   // DHT22: one reading every 2 s
  lastRead = millis();

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {               // NaN = "not a number" = no answer
    lcd.setCursor(0, 0);
    lcd.print("Sensor error!   ");
    return;
  }

  showLine(0, "Temp: ", t, " \337C  ");      // \337 = the LCD's degree sign
  showLine(1, "Hum:  ", h, " %   ");

  Serial.print(t, 1);
  Serial.print(" C   ");
  Serial.print(h, 1);
  Serial.println(" %");
}
