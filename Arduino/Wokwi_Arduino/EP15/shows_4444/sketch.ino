// Episode 15 - I want 1234 on four digits. One classic mistake.
const int SEG[7] = {2, 3, 4, 5, 6, 7, 8};   // segments a b c d e f g
const int DIG[4] = {9, 10, 11, 12};         // digits 1..4 (LOW = on)

const byte PATTERN[10] = {                  // bit 0 = a ... bit 6 = g
  0b0111111, 0b0000110, 0b1011011, 0b1001111, 0b1100110,
  0b1101101, 0b1111101, 0b0000111, 0b1111111, 0b1101111
};

void showNumber(int n) {
  for (int s = 0; s < 7; s++) digitalWrite(SEG[s], bitRead(PATTERN[n], s));
}

void setup() {
  for (int s = 0; s < 7; s++) pinMode(SEG[s], OUTPUT);
  for (int d = 0; d < 4; d++) pinMode(DIG[d], OUTPUT);

  int number[4] = {1, 2, 3, 4};
  for (int d = 0; d < 4; d++) {
    digitalWrite(DIG[d], LOW);     // switch digit d on...
    showNumber(number[d]);         // ...and draw its number
  }
}

void loop() {
}
