// Episode 20 - Reaction-time game, first try (can you find the cheat?)
const int LED = 9, BUTTON = 2;

void setup() {
  Serial.begin(9600);
  pinMode(LED, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  randomSeed(analogRead(A1));
}

void loop() {
  Serial.println("Get ready...");
  delay(random(2000, 5000));                // wait 2 to 5 seconds
  digitalWrite(LED, HIGH);                  // GO!
  unsigned long t0 = millis();
  while (digitalRead(BUTTON) == HIGH) {}    // wait for the press
  unsigned long ms = millis() - t0;
  digitalWrite(LED, LOW);
  Serial.print("Your time: ");
  Serial.print(ms);
  Serial.println(" ms");
  delay(2000);
}
