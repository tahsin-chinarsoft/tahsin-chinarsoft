// ChinarSoft - Arduino Sensor Kit, Episode 7
// Microphones: KY-037 (big, high sensitivity) and KY-038 (small)
// Project: Double-clap switch + sound level meter
//
// Wiring (Arduino Uno):
//   KY-037  AO -> A0 | DO -> D2 | + -> 5V | G -> GND
//   KY-038  AO -> A1 | DO -> D3 | + -> 5V | G -> GND
//   LED (or relay in Episode 12) -> D13 (on-board LED works too)
//
// Serial Plotter at 115200: "big:<p2p> small:<p2p> threshold:<value>"

const int MIC_BIG   = A0;
const int MIC_SMALL = A1;
const int OUT_PIN   = 13;

const unsigned long WINDOW_MS     = 20;    // measure peak-to-peak over 20 ms
const int CLAP_THRESHOLD          = 120;   // ADC steps peak-to-peak, tune on the plotter
const unsigned long CLAP_GAP_MIN  = 150;   // two claps must be at least this far apart
const unsigned long CLAP_GAP_MAX  = 700;   // ...and at most this far
const unsigned long CLAP_DEADTIME = 100;   // one clap lasts a while, don't count it twice

bool outputOn = false;
unsigned long firstClapMs = 0;
unsigned long lastClapMs = 0;

// The microphone output is an audio wave around a middle value.
// Loudness = (max - min) inside a short window.
void peakToPeak(int &big, int &small) {
  int bMin = 1023, bMax = 0, sMin = 1023, sMax = 0;
  unsigned long start = millis();
  while (millis() - start < WINDOW_MS) {
    int b = analogRead(MIC_BIG);
    int s = analogRead(MIC_SMALL);
    if (b < bMin) bMin = b;
    if (b > bMax) bMax = b;
    if (s < sMin) sMin = s;
    if (s > sMax) sMax = s;
  }
  big = bMax - bMin;
  small = sMax - sMin;
}

void onClap(unsigned long now) {
  unsigned long gap = now - firstClapMs;
  if (firstClapMs != 0 && gap >= CLAP_GAP_MIN && gap <= CLAP_GAP_MAX) {
    outputOn = !outputOn;
    digitalWrite(OUT_PIN, outputOn ? HIGH : LOW);
    Serial.println(outputOn ? "# DOUBLE CLAP -> ON" : "# DOUBLE CLAP -> OFF");
    firstClapMs = 0;
  } else {
    firstClapMs = now;               // this is the first clap of a possible pair
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(OUT_PIN, OUTPUT);
}

void loop() {
  int big, small;
  peakToPeak(big, small);
  unsigned long now = millis();

  if (big > CLAP_THRESHOLD && now - lastClapMs > CLAP_DEADTIME) {
    lastClapMs = now;
    onClap(now);
  }
  if (firstClapMs != 0 && now - firstClapMs > CLAP_GAP_MAX) firstClapMs = 0;  // single clap expired

  Serial.print("big:");   Serial.print(big);
  Serial.print(" small:"); Serial.print(small);
  Serial.print(" threshold:"); Serial.println(CLAP_THRESHOLD);
}
