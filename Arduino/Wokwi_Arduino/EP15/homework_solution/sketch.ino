// Episode 15 homework solution - stopwatch 0.0 to 999.9 on a 4-digit 7-segment display
const int SEG[7] = {2, 3, 4, 5, 6, 7, 8};   // segments a..g
const int DP = 13;                          // decimal point
const int DIG[4] = {9, 10, 11, 12};         // digits 1..4 (LOW = on)
const int BUTTON = A0;                      // to GND, INPUT_PULLUP

const byte PATTERN[10] = {
  0b0111111, 0b0000110, 0b1011011, 0b1001111, 0b1100110,
  0b1101101, 0b1111101, 0b0000111, 0b1111111, 0b1101111
};
const int PLACE[4] = {1000, 100, 10, 1};

bool running = false;
unsigned long tenths = 0;                   // 0 .. 9999 = 0.0 .. 999.9 s
unsigned long lastTick = 0;
bool lastButton = HIGH;
unsigned long lastChange = 0;
unsigned long lastSwitch = 0;
int current = 0;

void showDigit(int pos) {
  for (int d = 0; d < 4; d++) digitalWrite(DIG[d], HIGH);
  int n = (tenths / PLACE[pos]) % 10;
  bool blank = (pos < 2 && tenths < (unsigned long)PLACE[pos]);   // no leading zeros
  for (int s = 0; s < 7; s++) digitalWrite(SEG[s], blank ? LOW : bitRead(PATTERN[n], s));
  digitalWrite(DP, pos == 2);                                     // point after digit 3
  digitalWrite(DIG[pos], LOW);
}

void setup() {
  for (int s = 0; s < 7; s++) pinMode(SEG[s], OUTPUT);
  pinMode(DP, OUTPUT);
  for (int d = 0; d < 4; d++) pinMode(DIG[d], OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
}

void loop() {
  // button: start / stop
  bool b = digitalRead(BUTTON);
  if (b != lastButton && millis() - lastChange > 50) {
    lastChange = millis();
    lastButton = b;
    if (b == LOW) {
      running = !running;
      lastTick = millis();
    }
  }

  // time: one tenth every 100 ms
  if (running && millis() - lastTick >= 100) {
    lastTick += 100;
    if (tenths < 9999) tenths++;
  }

  // display: next digit every 2 ms
  if (millis() - lastSwitch >= 2) {
    lastSwitch = millis();
    current = (current + 1) % 4;
    showDigit(current);
  }
}
