// Episode 12 - first try: drive a servo with analogWrite()?
const int SERVO = 9;   // a ~PWM pin
const int POT = A0;

void setup() {
  Serial.begin(9600);
  pinMode(SERVO, OUTPUT);
}

void loop() {
  int raw = analogRead(POT);
  int angle = map(raw, 0, 1023, 0, 180);
  analogWrite(SERVO, angle);      // "90 should be 90 degrees..."
  Serial.print("I want ");
  Serial.print(angle);
  Serial.println(" degrees");
  delay(200);
}
