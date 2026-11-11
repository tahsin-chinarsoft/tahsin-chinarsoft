// Episode 14 - ultrasonic distance, with one classic mistake
const int TRIG = 9;
const int ECHO = 10;

void setup() {
  Serial.begin(9600);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
}

void loop() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);              // a 10 us pulse starts one measurement
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH);   // echo time in microseconds
  float cm = duration * 0.0343;          // sound: 0.0343 cm per microsecond
  Serial.print("echo ");
  Serial.print(duration);
  Serial.print(" us  ->  ");
  Serial.print(cm, 1);
  Serial.println(" cm");
  delay(500);
}
