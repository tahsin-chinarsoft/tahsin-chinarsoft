// Episode 17 - Rotary encoder read by polling, with a slow loop (the classic mistake)
const int CLK = 2, DT = 3;
int count = 0;
int lastClk = HIGH;

void setup() {
  Serial.begin(9600);
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
}

void loop() {
  int clk = digitalRead(CLK);
  if (clk != lastClk && clk == LOW) {        // CLK just fell: one click
    if (digitalRead(DT) == HIGH) count++;    // clockwise
    else count--;                            // counter-clockwise
  }
  lastClk = clk;

  Serial.print("Count: ");
  Serial.println(count);
  delay(200);                                // the rest of the program is busy
}
