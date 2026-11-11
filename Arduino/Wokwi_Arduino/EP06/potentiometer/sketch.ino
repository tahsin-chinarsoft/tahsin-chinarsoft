// Arduino in Your Browser — Episode 6: analogRead and the Potentiometer
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// The knob on A0 gives 0 ... 1023.
// We turn it into percent, real volts, and the LED's blink speed.

const int KNOB = A0;
const int LED = 12;

bool ledOn = false;
unsigned long lastBlink = 0;
unsigned long lastPrint = 0;

void setup() {
  pinMode(LED, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  int raw = analogRead(KNOB);                    // 0 ... 1023

  int percent = map(raw, 0, 1023, 0, 100);       // 0 ... 100 %
  float volts = raw * 5.0 / 1023.0;              // 0.00 ... 5.00 V
  int blinkMs = map(raw, 0, 1023, 1000, 50);     // left = slow, right = fast

  // blink with millis(), so the knob is read all the time
  if (millis() - lastBlink >= (unsigned long)blinkMs) {
    lastBlink = millis();
    ledOn = !ledOn;
    digitalWrite(LED, ledOn);
  }

  // print 5 times per second
  if (millis() - lastPrint >= 200) {
    lastPrint = millis();
    Serial.print("raw ");
    Serial.print(raw);
    Serial.print("  |  ");
    Serial.print(percent);
    Serial.print(" %  |  ");
    Serial.print(volts, 2);
    Serial.print(" V  |  blink ");
    Serial.print(blinkMs);
    Serial.println(" ms");
  }
}
