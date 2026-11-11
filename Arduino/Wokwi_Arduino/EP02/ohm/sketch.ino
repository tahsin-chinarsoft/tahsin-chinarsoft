// Arduino in Your Browser — Episode 2: Why Every LED Needs a Resistor
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// Wokwi does not model current, so an LED never burns in the simulator.
// This sketch teaches it Ohm's law:
//   the slider is a "virtual resistor" (10 to 1000 ohms),
//   the Arduino calculates the LED current, sets the brightness,
//   and burns the LED if the current is too high.
// Press the button to replace a burned LED.

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int SLIDER     = A0;   // slide potentiometer
const int LED_PIN    = 9;    // PWM pin, LED + 220 ohm (protects the real circuit)
const int REPLACE    = 2;    // button: replace the burned LED

const float SUPPLY_V = 5.0;  // Arduino pin voltage
const float LED_V    = 2.0;  // red LED forward voltage
const float SAFE_MA  = 20.0; // LED rating
const float BURN_MA  = 40.0; // above this for 1 s, the LED burns (pin absolute max)

LiquidCrystal_I2C lcd(0x27, 16, 2);
bool burned = false;
unsigned long overSince = 0;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(REPLACE, INPUT_PULLUP);
  lcd.init();
  lcd.backlight();
  Serial.begin(9600);
  Serial.println("Ohm's law: I = (5 V - 2 V) / R");
}

void loop() {
  // 1. Read the slider and turn it into a resistance
  int raw = analogRead(SLIDER);                 // 0 .. 1023
  float ohms = 10 + raw * (1000.0 - 10) / 1023; // 10 .. 1000 ohms

  // 2. Ohm's law: current = voltage across the resistor / resistance
  float mA = (SUPPLY_V - LED_V) / ohms * 1000;

  // 3. Too much current for too long? The LED burns.
  if (mA > BURN_MA) {
    if (overSince == 0) overSince = millis();
    if (millis() - overSince > 1000) burned = true;
  } else {
    overSince = 0;
  }
  if (digitalRead(REPLACE) == LOW) burned = false;   // new LED

  // 4. Brightness follows the current (20 mA = full brightness)
  int pwm = burned ? 0 : constrain(mA / SAFE_MA * 255, 0, 255);
  analogWrite(LED_PIN, pwm);

  // 5. Show it
  char cur[8], line[17];
  dtostrf(mA, 6, 1, cur);                       // e.g. "  13.6"
  snprintf(line, sizeof line, "R%4d  I%smA", (int)ohms, cur);
  lcd.setCursor(0, 0);
  lcd.print(line);
  lcd.setCursor(0, 1);
  if (burned)            lcd.print("BURNED! push btn");
  else if (mA > SAFE_MA) lcd.print("TOO HOT!        ");
  else if (mA < 5)       lcd.print("Safe, but dim   ");
  else                   lcd.print("Good - safe     ");

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    lastPrint = millis();
    Serial.print((int)ohms); Serial.print(" ohm -> ");
    Serial.print(mA, 1); Serial.println(burned ? " mA  BURNED" : " mA");
  }
  delay(50);
}
