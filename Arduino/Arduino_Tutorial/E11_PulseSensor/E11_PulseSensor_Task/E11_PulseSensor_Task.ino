// ChinarSoft - Arduino Sensor Kit, Episode 11 - TASK SOLUTION
// Task: beep the passive buzzer on every heartbeat, like a hospital monitor.
//
// What changed vs E11_PulseSensor:
//   - Passive buzzer on D8. Inside the beat detection, when a beat is ACCEPTED
//     (ibi between 300 and 2000 ms), tone(BUZZER_PIN, 1000, 60) plays a 1 kHz, 60 ms beep.
//     tone() with a duration runs in the background (Timer2), so the loop is not blocked.
//   - The sample timer now uses overflow-safe subtraction (millis() - lastSample)
//     instead of "millis() < nextSample" (which misbehaves after ~49 days).
//   - Filters, adaptive threshold, BPM estimate, LED and Serial Plotter output unchanged.
//
// Wiring: same wiring as E11_PulseSensor, plus ONE extra part:
//   KY-039   S -> A0 | middle -> 5V | - -> GND
//   LED      D13 (on-board) blinks on each beat
//   TASK: KY-006 passive buzzer  S -> D8 | - -> GND
//
// NOT a medical device.
// Use: put a fingertip gently between the IR LED and the sensor, keep still,
//      shade it from room light (a piece of black tape or a clothes peg helps).
// Serial Plotter at 115200: "pulse:<filtered> bpm:<value>"

const int SENSOR_PIN = A0;
const int LED_PIN = 13;
const int BUZZER_PIN = 8;                // TASK: passive buzzer
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
  pinMode(BUZZER_PIN, OUTPUT);           // TASK
  baseline = analogRead(SENSOR_PIN);
}

void loop() {
  static unsigned long lastSample = 0;   // TASK (fix): overflow-safe timer
  if (millis() - lastSample < SAMPLE_MS) return;
  lastSample = millis();

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
      tone(BUZZER_PIN, 1000, 60);                // TASK: 1 kHz, 60 ms = the hospital monitor beep
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
