// ChinarSoft - Arduino Sensor Kit, Episode 4
// Reed switches (KY-025, KY-021) and Hall sensors (KY-003, KY-035, KY-024)
// Project: Magnet lab - door sensor, RPM counter, magnet strength & polarity meter
//
// Wiring (Arduino Uno):
//   KY-003 Hall switch       S -> D2 (interrupt) | middle -> 5V | - -> GND
//   KY-021 mini reed         S -> D4
//   KY-025 reed module       DO -> D5 | AO -> (not used) | + -> 5V | G -> GND
//   KY-024 linear Hall       DO -> D6 | AO -> A1
//   KY-035 analog Hall       S -> A0  | middle -> 5V | - -> GND
//   Active buzzer            S -> D12
//
// Serial Plotter friendly: prints "field:<value> door:<0/1> rpm:<value>"

const int HALL_SWITCH_PIN = 2;
const int REED_MINI_PIN   = 4;
const int REED_MOD_PIN    = 5;
const int LIN_DO_PIN      = 6;
const int ANALOG_HALL_PIN = A0;
const int LIN_AO_PIN      = A1;
const int BUZZER_PIN      = 12;

volatile unsigned long pulseCount = 0;
volatile unsigned long lastPulseUs = 0;
int zeroField = 512;              // KY-035 reading with no magnet (calibrated at boot)
int reedIdle;
bool lastDoorOpen = false;

void onHallPulse() {
  unsigned long now = micros();
  if (now - lastPulseUs > 2000) {   // ignore pulses closer than 2 ms (max 30000 rpm)
    pulseCount++;
    lastPulseUs = now;
  }
}

int calibrateZero(int pin) {
  long sum = 0;
  for (int i = 0; i < 64; i++) { sum += analogRead(pin); delay(2); }
  return sum / 64;
}

void setup() {
  Serial.begin(115200);
  pinMode(HALL_SWITCH_PIN, INPUT_PULLUP);   // A3144 output is open-collector
  pinMode(REED_MINI_PIN, INPUT_PULLUP);
  pinMode(REED_MOD_PIN, INPUT);
  pinMode(LIN_DO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(HALL_SWITCH_PIN), onHallPulse, FALLING);

  // Keep magnets away during boot!
  zeroField = calibrateZero(ANALOG_HALL_PIN);
  reedIdle = digitalRead(REED_MINI_PIN);   // door must be CLOSED (magnet on reed) at boot
}

void loop() {
  static unsigned long lastReport = 0;
  static unsigned long lastCount = 0;
  static float rpm = 0;

  // Door sensor: mini reed switch with the magnet on the door
  bool doorOpen = digitalRead(REED_MINI_PIN) != reedIdle;
  if (doorOpen && !lastDoorOpen) {
    digitalWrite(BUZZER_PIN, HIGH); delay(60); digitalWrite(BUZZER_PIN, LOW);
  }
  lastDoorOpen = doorOpen;

  if (millis() - lastReport >= 250) {
    // RPM from Hall switch pulses (one magnet on the fan = 1 pulse per turn)
    noInterrupts();
    unsigned long count = pulseCount;
    interrupts();
    rpm = (count - lastCount) * 60000.0 / (millis() - lastReport);
    lastCount = count;
    lastReport = millis();

    // Analog field: sign = polarity, size = strength (raw ADC steps)
    int field = analogRead(ANALOG_HALL_PIN) - zeroField;

    Serial.print("field:");  Serial.print(field);
    Serial.print(" linearAO:"); Serial.print(analogRead(LIN_AO_PIN) - 512);
    Serial.print(" door:");  Serial.print(doorOpen ? 1 : 0);
    Serial.print(" reedModule:"); Serial.print(digitalRead(REED_MOD_PIN));
    Serial.print(" rpm:");   Serial.println(rpm, 0);
  }
}
