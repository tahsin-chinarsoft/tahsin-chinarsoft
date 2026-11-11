// Episode 17 homework solution - the menu wraps around, and a 4th item "Reset" restores the defaults
#include <LiquidCrystal_I2C.h>

const int CLK = 2, DT = 3, SW = 4;          // CLK on pin 2: interrupt pin
const int LED = 9, BUZZER = 8;

LiquidCrystal_I2C lcd(0x27, 20, 4);

volatile int steps = 0;                     // changed inside the interrupt

void onClk() {                              // runs on every CLK falling edge
  if (digitalRead(DT) == HIGH) steps++;
  else steps--;
}

const int N = 4;                            // 4 items: the 4th is "Reset"
const char* NAMES[N] = {"Brightness", "Blink speed", "Beep", "Reset"};
const int DEFAULTS[3] = {5, 5, 1};
int value[3] = {5, 5, 1};                   // 0..10, 1..10, 0/1
const int MINV[3] = {0, 1, 0}, MAXV[3] = {10, 10, 1};
int cursor = 0;
bool editing = false;

bool lastButton = HIGH;
unsigned long lastChange = 0;
unsigned long lastBlink = 0;
bool ledOn = true;

void drawMenu() {
  for (int i = 0; i < N; i++) {
    char line[21];
    char val[6] = "";
    if (i == 2) strcpy(val, value[i] ? "ON" : "OFF");
    else if (i < 2) itoa(value[i], val, 10);
    if (editing && i == cursor) snprintf(line, 21, "%c%-12s[%3s]  ", '>', NAMES[i], val);
    else snprintf(line, 21, "%c%-12s %3s   ", i == cursor ? '>' : ' ', NAMES[i], val);
    lcd.setCursor(0, i);
    lcd.print(line);                        // always the full 20 characters
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(SW, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(CLK), onClk, FALLING);
  lcd.init();
  lcd.backlight();
  drawMenu();
}

void loop() {
  // 1) take the steps the interrupt counted
  noInterrupts();
  int s = steps;
  steps = 0;
  interrupts();
  if (s != 0) {
    if (editing) value[cursor] = constrain(value[cursor] + s, MINV[cursor], MAXV[cursor]);
    else cursor = ((cursor + s) % N + N) % N;   // wrap around, also for negative steps
    if (value[2]) tone(BUZZER, 2000, 5);    // a small click
    drawMenu();
  }

  // 2) button: switch between moving and editing (debounced, Episode 4)
  bool b = digitalRead(SW);
  if (b != lastButton && millis() - lastChange > 50) {
    lastChange = millis();
    lastButton = b;
    if (b == LOW) {
      if (cursor == 3) {                    // "Reset": back to the defaults
        for (int i = 0; i < 3; i++) value[i] = DEFAULTS[i];
      } else {
        editing = !editing;
      }
      drawMenu();
    }
  }

  // 3) the LED uses the settings: brightness and blink speed
  unsigned long gap = 1100 - value[1] * 100;  // speed 1 = 1000 ms ... 10 = 100 ms
  if (millis() - lastBlink >= gap) {
    lastBlink = millis();
    ledOn = !ledOn;
  }
  analogWrite(LED, ledOn ? map(value[0], 0, 10, 0, 255) : 0);
}
