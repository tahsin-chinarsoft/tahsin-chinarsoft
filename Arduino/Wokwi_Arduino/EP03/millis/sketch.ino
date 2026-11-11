// Arduino in Your Browser — Episode 3: Traffic Light with millis()
// ChinarSoft | Wokwi Arduino Uno, no hardware needed
//
// No delay() anywhere. loop() runs thousands of times per second,
// checks the clock, and changes the light only when its time is up.
// The pedestrian button works at any moment, and the built-in LED
// keeps blinking to prove the Arduino is never stuck.

const int RED = 12, YELLOW = 11, GREEN = 10;
const int BUTTON = 2;                       // pedestrian button

const unsigned long GREEN_MS  = 5000;
const unsigned long YELLOW_MS = 1500;
const unsigned long RED_MS    = 4000;
const unsigned long BLINK_MS  = 250;        // heartbeat

enum Light { GREEN_ON, YELLOW_ON, RED_ON };
Light light = GREEN_ON;
unsigned long lightStart = 0;               // when the current light began
unsigned long lastBlink = 0;

void show(Light l) {
  light = l;
  lightStart = millis();
  digitalWrite(RED,    l == RED_ON);
  digitalWrite(YELLOW, l == YELLOW_ON);
  digitalWrite(GREEN,  l == GREEN_ON);
  Serial.println(l == GREEN_ON ? "GREEN" : l == YELLOW_ON ? "YELLOW" : "RED");
}

void setup() {
  pinMode(RED, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  Serial.begin(9600);
  show(GREEN_ON);
}

void loop() {
  unsigned long now = millis();

  // 1. Heartbeat: blinks every 250 ms, so we can see loop() never stops
  if (now - lastBlink >= BLINK_MS) {
    lastBlink = now;
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  // 2. Pedestrian button: checked on every pass through loop()
  if (light == GREEN_ON && digitalRead(BUTTON) == LOW && now - lightStart > 1000) {
    Serial.println("Button! Ending green early");
    show(YELLOW_ON);
  }

  // 3. Timers: is it time for the next light?
  unsigned long elapsed = now - lightStart;
  if      (light == GREEN_ON  && elapsed >= GREEN_MS)  show(YELLOW_ON);
  else if (light == YELLOW_ON && elapsed >= YELLOW_MS) show(RED_ON);
  else if (light == RED_ON    && elapsed >= RED_MS)    show(GREEN_ON);
}
