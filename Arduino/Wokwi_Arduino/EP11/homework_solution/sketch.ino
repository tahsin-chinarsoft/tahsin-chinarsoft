// Episode 11 homework solution: a siren with millis() and no delay()
const int BUZZER = 8;

int freq = 400;          // current frequency in Hz
int step = 10;           // +10 Hz going up, -10 Hz going down
unsigned long lastStep = 0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  if (millis() - lastStep >= 10) {     // one small step every 10 ms
    lastStep = millis();
    freq += step;
    if (freq >= 1000 || freq <= 400) {
      step = -step;                      // turn around at the ends
    }
    tone(BUZZER, freq);                  // no duration: keep playing
  }
  // loop() is free here: buttons, sensors, LEDs...
}
