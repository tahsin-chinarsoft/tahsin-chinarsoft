// ChinarSoft - Arduino Sensor Kit, Episode 0 - TASK SOLUTION
// Task: blink your initials in Morse code on the on-board LED, using flash(DOT) and flash(DASH).
//
// Changes vs E00_Blink:
//   - New BLINK_MODE 3 = "initials in Morse" (now the default). Modes 0, 1, 2 work as before.
//   - A small Morse table for A-Z and a blinkInitials() function.
//   - Put YOUR initials in INITIALS below (letters A-Z, a space = gap between words).
//   - Timing: dot = 200 ms, dash = 600 ms (3 dots), gap inside a letter = 1 dot,
//     gap between letters = 3 dots, gap after the whole word = 7 dots.
//
// Wiring: same wiring as E00_Blink (no wiring, on-board LED on pin 13).
// Open the Serial Monitor at 9600 baud to see each letter as it is sent.

const int LED_PIN = LED_BUILTIN;      // = 13 on the Uno
const int BLINK_MODE = 3;             // TASK: 3 = initials in Morse

const int DOT = 200;                  // Morse timing in ms
const int DASH = DOT * 3;

const char* INITIALS = "TS";          // TASK: change to your own initials, e.g. "AB"

// TASK: Morse code for A..Z ('.' = dot, '-' = dash)
const char* const MORSE[26] = {
  ".-",   "-...", "-.-.", "-..",  ".",    "..-.", "--.",  "....", "..",   // A-I
  ".---", "-.-",  ".-..", "--",   "-.",   "---",  ".--.", "--.-", ".-.",  // J-R
  "...",  "-",    "..-",  "...-", ".--",  "-..-", "-.--", "--.."          // S-Z
};

// setup() runs ONCE, after power-up or reset
void setup() {
  pinMode(LED_PIN, OUTPUT);           // pin 13 is an output
  Serial.begin(9600);                 // start the USB serial link
  Serial.println("Hello from Arduino! Episode 0");
  Serial.print("Blink mode: ");
  Serial.println(BLINK_MODE);
}

void flash(int onMs) {
  digitalWrite(LED_PIN, HIGH);        // 5 V on pin 13 -> LED on
  delay(onMs);
  digitalWrite(LED_PIN, LOW);         // 0 V -> LED off
  delay(DOT);                         // gap between symbols
}

void blinkClassic() {
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED ON");
  delay(1000);                        // wait 1000 ms = 1 s (the board does nothing else)
  digitalWrite(LED_PIN, LOW);
  Serial.println("LED OFF");
  delay(1000);
}

void blinkSOS() {
  for (int i = 0; i < 3; i++) flash(DOT);    // S = ...
  delay(DOT * 2);
  for (int i = 0; i < 3; i++) flash(DASH);   // O = ---
  delay(DOT * 2);
  for (int i = 0; i < 3; i++) flash(DOT);    // S = ...
  Serial.println("SOS sent");
  delay(DOT * 7);                            // gap between words
}

// TASK: send one letter (A-Z or a-z) with flash(DOT) / flash(DASH)
void sendLetter(char c) {
  if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';   // lower case works too
  if (c < 'A' || c > 'Z') return;                // skip anything that is not a letter
  const char* code = MORSE[c - 'A'];
  Serial.print(c); Serial.print("  "); Serial.println(code);
  for (int i = 0; code[i] != '\0'; i++) {
    if (code[i] == '.') flash(DOT);
    else flash(DASH);
  }
  delay(DOT * 2);                            // + 1 dot inside flash() = 3 dots between letters
}

// TASK: blink the whole INITIALS string, then pause
void blinkInitials() {
  for (int i = 0; INITIALS[i] != '\0'; i++) {
    if (INITIALS[i] == ' ') delay(DOT * 4);  // word gap inside the string (3 + 4 = 7 dots)
    else sendLetter(INITIALS[i]);
  }
  Serial.println("Initials sent");
  delay(DOT * 7);                            // gap before repeating
}

void blinkWithoutDelay() {
  static unsigned long lastToggle = 0;
  static bool ledOn = false;
  if (millis() - lastToggle >= 1000) {       // has 1 s passed?
    lastToggle = millis();
    ledOn = !ledOn;
    digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
    Serial.print("t = ");
    Serial.print(millis());
    Serial.println(ledOn ? " ms  LED ON" : " ms  LED OFF");
  }
  // ...the loop keeps running here: you could read sensors at the same time
}

// loop() runs AGAIN and AGAIN, forever
void loop() {
  if (BLINK_MODE == 0) blinkClassic();
  else if (BLINK_MODE == 1) blinkSOS();
  else if (BLINK_MODE == 3) blinkInitials();   // TASK
  else blinkWithoutDelay();
}
