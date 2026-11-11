// Episode 15 - 4-digit seven-segment counter with multiplexing
const int SEG[7] = {2, 3, 4, 5, 6, 7, 8};   // segments a b c d e f g
const int DIG[4] = {9, 10, 11, 12};         // digits 1..4 (LOW = on)
const int BUTTON = A0;                      // to GND, INPUT_PULLUP

const byte PATTERN[10] = {                  // bit 0 = a ... bit 6 = g
  0b0111111, 0b0000110, 0b1011011, 0b1001111, 0b1100110,
  0b1101101, 0b1111101, 0b0000111, 0b1111111, 0b1101111
};
const int PLACE[4] = {1000, 100, 10, 1};

int count = 1234;
int current = 0;                            // the one digit that is lit now
bool lastButton = HIGH;
unsigned long lastChange = 0;
unsigned long lastSwitch = 0;

void showDigit(int pos, int n) {
  for (int d = 0; d < 4; d++) digitalWrite(DIG[d], HIGH);   // all digits off
  for (int s = 0; s < 7; s++) digitalWrite(SEG[s], bitRead(PATTERN[n], s));
  digitalWrite(DIG[pos], LOW);                              // only this one on
}

void setup() {
  Serial.begin(9600);
  for (int s = 0; s < 7; s++) pinMode(SEG[s], OUTPUT);
  for (int d = 0; d < 4; d++) pinMode(DIG[d], OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
}

void loop() {
  // 1) button: every press adds one
  bool b = digitalRead(BUTTON);
  if (b != lastButton && millis() - lastChange > 50) {
    lastChange = millis();
    lastButton = b;
    if (b == LOW) {
      count = (count + 1) % 10000;
      Serial.println(count);
    }
  }

  // 2) display: next digit every 2 ms
  if (millis() - lastSwitch >= 2) {
    lastSwitch = millis();
    current = (current + 1) % 4;
    showDigit(current, (count / PLACE[current]) % 10);
  }
}
