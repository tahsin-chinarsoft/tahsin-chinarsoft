// Arduino in Your Browser — Episode 8 HOMEWORK SOLUTION: night lamp that FADES (with hysteresis)
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// Same hysteresis as Episode 8 (on above 650, off below 550),
// but the lamp ramps its brightness over about one second. No delay().

const int SENSOR = A0;
const int LAMP = 9;               // ~ PWM pin
const int ON_ABOVE  = 650;
const int OFF_BELOW = 550;

bool lampOn = false;
int brightness = 0;               // 0 ... 255
unsigned long lastStep = 0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  int light = analogRead(SENSOR);

  if (!lampOn && light > ON_ABOVE)  { lampOn = true;  Serial.println("dark  -> fade in");  }
  if ( lampOn && light < OFF_BELOW) { lampOn = false; Serial.println("light -> fade out"); }

  int target = lampOn ? 255 : 0;
  if (brightness != target && millis() - lastStep >= 4) {   // 255 steps x 4 ms = ~1 s
    lastStep = millis();
    brightness += (target > brightness) ? 1 : -1;
    analogWrite(LAMP, brightness);
  }
}
