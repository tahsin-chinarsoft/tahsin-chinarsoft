// Arduino in Your Browser — Episode 9 HOMEWORK SOLUTION: NTC + fan + warning light
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// Fan LED on pin 9: ON above 30 C, OFF below 28 C (hysteresis).
// Built-in LED on pin 13 blinks (250 ms) above 50 C. No delay() anywhere.

const int SENSOR = A0;
const int FAN = 9;
const int WARN = 13;
const float BETA = 3950;
const float T0 = 298.15;

bool fanOn = false;
bool warnOn = false;
unsigned long lastBlink = 0;
unsigned long lastPrint = 0;

void setup() {
  Serial.begin(9600);
  pinMode(FAN, OUTPUT);
  pinMode(WARN, OUTPUT);
}

void loop() {
  int raw = analogRead(SENSOR);
  float ratio = 1023.0 / raw - 1;
  float celsius = 1 / (log(1 / ratio) / BETA + 1 / T0) - 273.15;

  if (!fanOn && celsius > 30) { fanOn = true;  digitalWrite(FAN, HIGH); }
  if ( fanOn && celsius < 28) { fanOn = false; digitalWrite(FAN, LOW);  }

  if (celsius > 50) {
    if (millis() - lastBlink >= 250) {       // blink: flip every 250 ms
      lastBlink = millis();
      warnOn = !warnOn;
      digitalWrite(WARN, warnOn);
    }
  } else if (warnOn) {
    warnOn = false;
    digitalWrite(WARN, LOW);
  }

  if (millis() - lastPrint >= 500) {
    lastPrint = millis();
    Serial.print(celsius, 1);
    Serial.print(" C   fan ");
    Serial.print(fanOn ? "ON" : "off");
    Serial.println(celsius > 50 ? "   WARNING" : "");
  }
}
