// Arduino in Your Browser — Episode 10: DHT22 weather station, FIRST TRY (ghost characters!)
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// DHT22 data -> pin 2. LCD1602 I2C: SDA -> A4, SCL -> A5 (address 0x27).
// Click the DHT22 and lower the temperature from 10 to 9.5 ... watch the LCD.

#include <DHT.h>
#include <LiquidCrystal_I2C.h>

DHT dht(2, DHT22);
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.print("Weather station");
  delay(1000);
}

void loop() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(t, 1);
  lcd.print(" C");

  lcd.setCursor(0, 1);
  lcd.print("Hum: ");
  lcd.print(h, 1);
  lcd.print(" %");

  delay(2000);
}
