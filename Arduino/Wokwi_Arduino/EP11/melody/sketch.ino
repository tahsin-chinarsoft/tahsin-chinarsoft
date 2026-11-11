// Arduino in Your Browser — Episode 11: play a melody with tone()
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// Buzzer: pin 1 -> GND, pin 2 -> pin 8.
// Twinkle, Twinkle, Little Star: each note gets its time, then a tiny gap.

const int BUZZER = 8;

const int NOTE_C4 = 262, NOTE_D4 = 294, NOTE_E4 = 330;
const int NOTE_F4 = 349, NOTE_G4 = 392, NOTE_A4 = 440;

int melody[] = {NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_G4,
                NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_C4};
int durations[] = {400, 400, 400, 400, 400, 400, 800,
                   400, 400, 400, 400, 400, 400, 800};
const int NOTES = 14;

void setup() {
  Serial.begin(9600);
}

void loop() {
  for (int i = 0; i < NOTES; i++) {
    tone(BUZZER, melody[i], durations[i] * 0.9);   // 90 % sound ...
    Serial.println(melody[i]);
    delay(durations[i]);                           // ... and wait for the whole note
  }
  delay(2000);
}
