// Episode 14 - ultrasonic distance meter on an I2C LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int TRIG = 9;
const int ECHO = 10;
const unsigned long TIMEOUT_US = 17500;   // about 3 m, then give up

LiquidCrystal_I2C lcd(0x27, 16, 2);
unsigned long lastRead = 0;

void setup() {
  Serial.begin(9600);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  lcd.init();
  lcd.backlight();
  lcd.print("Distance:");
}

float readCm() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long duration = pulseIn(ECHO, HIGH, TIMEOUT_US);
  if (duration == 0) return -1;            // no echo in time
  return duration * 0.0343 / 2;            // there AND back: divide by 2
}

void loop() {
  if (millis() - lastRead >= 200) {
    lastRead = millis();
    float cm = readCm();
    lcd.setCursor(0, 1);
    if (cm < 0) {
      lcd.print("out of range    ");      // always 16 characters
      Serial.println("out of range");
    } else {
      char num[8];
      dtostrf(cm, 6, 1, num);              // e.g. " 100.0"
      lcd.print(num);
      lcd.print(" cm       ");
      Serial.print(cm, 1);
      Serial.println(" cm");
    }
  }
}
