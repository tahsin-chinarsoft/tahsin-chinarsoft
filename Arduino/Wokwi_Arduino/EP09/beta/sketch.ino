// Arduino in Your Browser — Episode 9: real degrees from an NTC (Beta equation) + a fan
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// NTC module OUT -> A0 (10k NTC, Beta 3950, 10k fixed resistor).
// Fan LED on pin 9: ON above 30 C, OFF below 28 C (hysteresis, Episode 8).

const int SENSOR = A0;
const int FAN = 9;
const float BETA = 3950;          // from the NTC datasheet
const float T0 = 298.15;          // 25 C in kelvin

bool fanOn = false;

void setup() {
  Serial.begin(9600);
  pinMode(FAN, OUTPUT);
}

void loop() {
  int raw = analogRead(SENSOR);

  float ratio = 1023.0 / raw - 1;                         // R_fixed / R_ntc
  float kelvin = 1 / (log(1 / ratio) / BETA + 1 / T0);    // Beta equation
  float celsius = kelvin - 273.15;

  if (!fanOn && celsius > 30) { fanOn = true;  digitalWrite(FAN, HIGH); }
  if ( fanOn && celsius < 28) { fanOn = false; digitalWrite(FAN, LOW);  }

  Serial.print("raw ");
  Serial.print(raw);
  Serial.print("  ->  ");
  Serial.print(celsius, 1);
  Serial.print(" C   fan ");
  Serial.println(fanOn ? "ON" : "off");
  delay(500);
}
