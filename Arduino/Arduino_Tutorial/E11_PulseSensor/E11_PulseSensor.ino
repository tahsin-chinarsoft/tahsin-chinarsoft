// ChinarSoft - Arduino Sensor Kit, Episode 11
// Finger detection / heartbeat module (KY-039)
// Project: Pulse plotter with BPM estimate (NOT a medical device)
//
// Wiring (Arduino Uno):
//   KY-039   S -> A0 | middle -> 5V | - -> GND
//   LED      D13 (on-board) blinks on each beat
//
// Use: put a fingertip gently between the IR LED and the sensor, keep still,
//      shade it from room light (a piece of black tape or a clothes peg helps).
// Serial Plotter at 115200: "pulse:<filtered> bpm:<value>"

const int SENSOR_PIN = A0;
const int LED_PIN = 13;
const unsigned long SAMPLE_MS = 20;      // 50 samples per second

float baseline = 0;     // slow average = DC level (blood + tissue + ambient light)
float smooth = 0;       // fast average = removes high-frequency noise
float envelope = 1;     // recent signal size, for an adaptive threshold
bool above = false;
unsigned long lastBeatMs = 0;
float bpm = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  baseline = analogRead(SENSOR_PIN);
}

void loop() {
  static unsigned long nextSample = 0;
  if (millis() < nextSample) return;
  nextSample = millis() + SAMPLE_MS;

  int raw = analogRead(SENSOR_PIN);

  // 1) Remove the big DC level: what is left is the tiny pulse wave
  baseline += (raw - baseline) * 0.02;          // ~1 s time constant
  float ac = raw - baseline;

  // 2) Smooth the AC part
  smooth += (ac - smooth) * 0.3;

  // 3) Track how big the wave is, so the threshold adapts to each finger
  envelope = max(envelope * 0.995f, fabs(smooth));
  float threshold = envelope * 0.5;

  // KY-039: more blood = LESS light = lower reading, so a beat is a DIP.
  float signal = -smooth;

  if (!above && signal > threshold) {
    above = true;
    unsigned long now = millis();
    unsigned long ibi = now - lastBeatMs;        // inter-beat interval
    if (ibi > 300 && ibi < 2000) {               // 30..200 BPM is plausible
      float instant = 60000.0 / ibi;
      bpm = (bpm == 0) ? instant : bpm * 0.7 + instant * 0.3;
      digitalWrite(LED_PIN, HIGH);
    }
    lastBeatMs = now;
  }
  if (above && signal < threshold * 0.3) {
    above = false;
    digitalWrite(LED_PIN, LOW);
  }
  if (millis() - lastBeatMs > 3000) bpm = 0;     // no finger / lost signal

  Serial.print("pulse:"); Serial.print(signal, 2);
  Serial.print(" threshold:"); Serial.print(threshold, 2);
  Serial.print(" bpm:"); Serial.println(bpm, 0);
}
