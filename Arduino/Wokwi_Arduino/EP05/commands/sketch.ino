// Arduino in Your Browser — Episode 5: Serial Monitor (commands)
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// Talk to your Arduino. Type a command and press Enter:
//   on          -> LED on
//   off         -> LED off
//   blink 200   -> blink every 200 ms (no delay!)
//   status      -> what is the LED doing?

const int LED = 12;

bool ledOn = false;
bool blinking = false;
unsigned long blinkMs = 500;
unsigned long lastBlink = 0;

void setLed(bool on) {
  ledOn = on;
  digitalWrite(LED, on);
}

void setup() {
  pinMode(LED, OUTPUT);
  Serial.begin(9600);
  Serial.println("Commands: on, off, blink <ms>, status");
}

void loop() {
  // 1. A whole line arrived? Read it as text.
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');   // read up to Enter
    cmd.trim();                                  // remove spaces, \r, \n
    cmd.toLowerCase();

    // 2. Compare the text and act.
    if (cmd == "on") {
      blinking = false;
      setLed(true);
      Serial.println("LED is ON");
    } else if (cmd == "off") {
      blinking = false;
      setLed(false);
      Serial.println("LED is OFF");
    } else if (cmd.startsWith("blink")) {
      int ms = cmd.substring(5).toInt();         // "blink 200" -> 200
      if (ms < 50) ms = 500;                     // missing or too fast
      blinkMs = ms;
      blinking = true;
      Serial.print("Blinking every ");
      Serial.print(ms);
      Serial.println(" ms");
    } else if (cmd == "status") {
      Serial.print("LED is ");
      Serial.print(blinking ? "blinking" : (ledOn ? "ON" : "OFF"));
      if (blinking) { Serial.print(" every "); Serial.print(blinkMs); Serial.print(" ms"); }
      Serial.println();
    } else if (cmd.length() > 0) {
      Serial.print("Unknown command: ");
      Serial.println(cmd);
    }
  }

  // 3. Blinking runs with millis(), so commands still work any time.
  if (blinking && millis() - lastBlink >= blinkMs) {
    lastBlink = millis();
    setLed(!ledOn);
  }
}
