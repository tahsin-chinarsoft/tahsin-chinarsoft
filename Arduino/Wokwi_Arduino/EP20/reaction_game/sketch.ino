// Episode 20 - Capstone: reaction-time game with a state machine (button, LED, buzzer, OLED)
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int LED = 9, BUTTON = 2, BUZZER = 8;
Adafruit_SSD1306 display(128, 64, &Wire, -1);

enum State { IDLE, WAITING, GO, RESULT };
State state = IDLE;
unsigned long stateStart = 0;               // when we entered this state
unsigned long waitTime = 0;                 // random 2..5 s
unsigned long result = 0;
unsigned long best = 0;                     // 0 = no best yet
bool tooEarly = false;

bool lastButton = HIGH;
unsigned long lastChange = 0;

void show(const char* line1, const char* line2) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(line1);
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.print(line2);
  display.setTextSize(1);
  display.setCursor(0, 54);
  display.print("Best: ");
  if (best) { display.print(best); display.print(" ms"); }
  else display.print("--");
  display.display();
}

void enter(State s) {
  state = s;
  stateStart = millis();
  char buf[16];
  switch (s) {
    case IDLE:    show("Reaction game", "Press!"); break;
    case WAITING: waitTime = random(2000, 5000); show("Wait for the light", "..."); break;
    case GO:      digitalWrite(LED, HIGH); tone(BUZZER, 1500, 80); show("NOW!", "GO!"); break;
    case RESULT:
      digitalWrite(LED, LOW);
      if (tooEarly) show("Press to retry", "Too early!");
      else { snprintf(buf, 16, "%lu ms", result); show("Your time", buf); }
      break;
  }
}

bool pressed() {                            // true once per press (debounced, Episode 4)
  bool b = digitalRead(BUTTON);
  if (b != lastButton && millis() - lastChange > 20) {
    lastChange = millis();
    lastButton = b;
    return b == LOW;
  }
  return false;
}

void setup() {
  Serial.begin(9600);
  pinMode(LED, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  randomSeed(analogRead(A1));
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(SSD1306_WHITE);
  enter(IDLE);
}

void loop() {
  bool p = pressed();
  switch (state) {
    case IDLE:
      if (p) { tooEarly = false; enter(WAITING); }
      break;
    case WAITING:
      if (p) { tooEarly = true; Serial.println("Too early"); enter(RESULT); }   // pressed before the light
      else if (millis() - stateStart >= waitTime) enter(GO);
      break;
    case GO:
      if (p) {
        result = millis() - stateStart;
        if (best == 0 || result < best) best = result;
        Serial.print("Time: "); Serial.println(result);
        enter(RESULT);
      }
      break;
    case RESULT:
      if (p) { tooEarly = false; enter(WAITING); }
      break;
  }
}
