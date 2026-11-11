// Episode 16 - Reading a 4x4 keypad by hand (matrix scanning, no library)
const int ROWS[4] = {9, 8, 7, 6};      // R1..R4 - outputs
const int COLS[4] = {5, 4, 3, 2};      // C1..C4 - inputs with pull-up
const char KEYS[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

char lastKey = 0;

char scanKeypad() {
  for (int r = 0; r < 4; r++) {
    for (int i = 0; i < 4; i++) digitalWrite(ROWS[i], HIGH);  // all rows HIGH
    digitalWrite(ROWS[r], LOW);                               // pull one row LOW
    for (int c = 0; c < 4; c++) {
      if (digitalRead(COLS[c]) == LOW) return KEYS[r][c];     // pressed key connects them
    }
  }
  return 0;                                                   // nothing pressed
}

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 4; i++) pinMode(ROWS[i], OUTPUT);
  for (int i = 0; i < 4; i++) pinMode(COLS[i], INPUT_PULLUP);
  Serial.println("Press a key");
}

void loop() {
  char key = scanKeypad();
  if (key != 0 && key != lastKey) {   // print once per press
    Serial.print("Key: ");
    Serial.println(key);
  }
  lastKey = key;
  delay(10);
}
