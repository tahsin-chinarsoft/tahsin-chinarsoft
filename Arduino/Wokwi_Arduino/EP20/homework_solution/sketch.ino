// Episode 20 final challenge solution: best of five (average of 5 valid rounds, too-early rounds do not count)
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int LED = 9, BUTTON = 2, BUZZER = 8;
const int ROUNDS = 5;
Adafruit_SSD1306 display(128, 64, &Wire, -1);

enum State { IDLE, WAITING, GO, RESULT, FINAL };
State state = IDLE;
unsigned long stateStart = 0;
unsigned long waitTime = 0;
unsigned long result = 0;
unsigned long total = 0;                    // sum of valid times
int done = 0;                               // valid rounds so far
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
  display.print("Round ");
  display.print(min(done + 1, ROUNDS));
  display.print(" of ");
  display.print(ROUNDS);
  display.display();
}

void enter(State s) {
  state = s;
  stateStart = millis();
  char buf[16];
  switch (s) {
    case IDLE:    show("Best of five", "Press!"); break;
    case WAITING: waitTime = random(2000, 5000); show("Wait for the light", "..."); break;
    case GO:      digitalWrite(LED, HIGH); tone(BUZZER, 1500, 80); show("NOW!", "GO!"); break;
    case RESULT:
      digitalWrite(LED, LOW);
      if (tooEarly) show("Not counted, retry", "Too early!");
      else { snprintf(buf, 16, "%lu ms", result); show("Your time", buf); }
      break;
    case FINAL:
      digitalWrite(LED, LOW);
      snprintf(buf, 16, "%lu ms", total / ROUNDS);
      display.clearDisplay();
      display.setTextSize(1); display.setCursor(0, 0); display.print("Average of 5");
      display.setTextSize(2); display.setCursor(0, 20); display.print(buf);
      display.setTextSize(1); display.setCursor(0, 54); display.print("Press to play again");
      display.display();
      Serial.print("Average: "); Serial.println(total / ROUNDS);
      break;
  }
}

bool pressed() {                            // true once per press (debounced)
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
      if (p) { tooEarly = true; Serial.println("Too early (not counted)"); enter(RESULT); }
      else if (millis() - stateStart >= waitTime) enter(GO);
      break;
    case GO:
      if (p) {
        result = millis() - stateStart;
        total += result;
        done++;
        Serial.print("Round "); Serial.print(done); Serial.print(": "); Serial.println(result);
        enter(RESULT);
      }
      break;
    case RESULT:
      if (p) {
        tooEarly = false;
        if (done >= ROUNDS) enter(FINAL);
        else enter(WAITING);
      }
      break;
    case FINAL:
      if (p) { done = 0; total = 0; enter(IDLE); }
      break;
  }
}
