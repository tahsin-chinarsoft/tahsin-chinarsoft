// Arduino in Your Browser — Episode 11: my 8-note melody plays only ONE note
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// Buzzer: pin 1 -> GND, pin 2 -> pin 8.
// tone() starts a note and returns immediately. The next tone() replaces it.

const int BUZZER = 8;
int melody[] = {262, 294, 330, 349, 392, 440, 494, 523};   // C D E F G A B C

void setup() {
  Serial.begin(9600);
}

void loop() {
  for (int i = 0; i < 8; i++) {
    tone(BUZZER, melody[i], 400);    // start the note ... and move on!
  }
  Serial.println("Melody done?");
  delay(3000);
}
