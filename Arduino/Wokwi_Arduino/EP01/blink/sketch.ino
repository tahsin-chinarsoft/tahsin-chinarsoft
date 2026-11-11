// Arduino in Your Browser — Episode 1: Your First Circuit
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// The built-in LED (pin 13) and an external LED (pin 12)
// blink together. Change BLINK_MS and press Play again.

const int EXTERNAL_LED = 12;   // LED + 220 ohm resistor
const int BLINK_MS     = 500;  // on time and off time, in milliseconds

// setup() runs ONCE, when the board powers up
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(EXTERNAL_LED, OUTPUT);
  Serial.begin(9600);
  Serial.println("Hello from a virtual Arduino!");
}

// loop() runs FOREVER, over and over
void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(EXTERNAL_LED, HIGH);
  Serial.println("ON");
  delay(BLINK_MS);

  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(EXTERNAL_LED, LOW);
  Serial.println("OFF");
  delay(BLINK_MS);
}
