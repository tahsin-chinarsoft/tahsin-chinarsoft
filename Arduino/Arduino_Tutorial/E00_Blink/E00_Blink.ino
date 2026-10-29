// ChinarSoft - Arduino Sensor Kit, Episode 0
// Arduino basics: the on-board LED (pin 13, "L") - your first program
//
// No wiring needed: the Uno has an LED on pin 13, labelled "L".
// Change BLINK_MODE and upload again to try each version:
//   0 = classic blink (1 s on, 1 s off)
//   1 = SOS in Morse code ( ... --- ... )
//   2 = blink without delay() using millis()  (the Arduino stays free to do other work)
// Open the Serial Monitor at 9600 baud to see what the board is doing.

const int LED_PIN = LED_BUILTIN;      // = 13 on the Uno
const int BLINK_MODE = 0;

const int DOT = 200;                  // Morse timing in ms
const int DASH = DOT * 3;

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
  else blinkWithoutDelay();
}
