// ChinarSoft - Arduino Sensor Kit, Episode 7 - TASK SOLUTION
// Task: three claps toggle a second LED (two claps still toggle the first one).
//
// What changed vs E07_ClapSwitch:
//   - The "pair of claps" logic (onClap / firstClapMs) is replaced by a clap COUNTER:
//     every clap does clapCount++ and lastClapMs = now.
//   - When the room has been quiet for 700 ms (CLAP_WINDOW_MS) we decide:
//       2 claps -> toggle LED1 (the original output on D13)
//       3 claps -> toggle LED2 (new, D8)
//     Any other count (1, 4, 5...) is ignored.
//   - Note: a double clap now acts ~0.7 s after the last clap, because we must wait
//     to be sure a third clap is not coming.
//   - Sound level meter / Serial Plotter output unchanged.
//
// Wiring: same wiring as E07_ClapSwitch, plus ONE extra part:
//   KY-037  AO -> A0 | DO -> D2 | + -> 5V | G -> GND
//   KY-038  AO -> A1 | DO -> D3 | + -> 5V | G -> GND
//   LED1 (or relay in Episode 12) -> D13 (on-board LED works too)
//   TASK: LED2 -> D8 through a 220 ohm resistor to GND (any free LED / KY-011 / KY-016 color)
//
// Serial Plotter at 115200: "big:<p2p> small:<p2p> threshold:<value>"

const int MIC_BIG   = A0;
const int MIC_SMALL = A1;
const int LED1      = 13;   // TASK: was OUT_PIN, same pin
const int LED2      = 8;    // TASK: second output

const unsigned long WINDOW_MS      = 20;    // measure peak-to-peak over 20 ms
const int CLAP_THRESHOLD           = 120;   // ADC steps peak-to-peak, tune on the plotter
const unsigned long CLAP_DEADTIME  = 100;   // one clap lasts a while, don't count it twice
const unsigned long CLAP_WINDOW_MS = 700;   // TASK: quiet this long = the clap sequence is over

int clapCount = 0;                          // TASK: claps in the current sequence
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

// TASK: toggle any output pin and report it
void toggle(int pin) {
  bool on = !digitalRead(pin);              // reading an OUTPUT pin returns its current state
  digitalWrite(pin, on ? HIGH : LOW);
  Serial.print(pin == LED1 ? "# DOUBLE CLAP -> LED1 " : "# TRIPLE CLAP -> LED2 ");
  Serial.println(on ? "ON" : "OFF");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);                    // TASK
}

void loop() {
  int big, small;
  peakToPeak(big, small);
  unsigned long now = millis();

  // TASK: count claps inside the window
  if (big > CLAP_THRESHOLD && now - lastClapMs > CLAP_DEADTIME) {
    clapCount++;
    lastClapMs = now;
  }

  // TASK: window closed (quiet for 700 ms): decide
  if (clapCount > 0 && now - lastClapMs > CLAP_WINDOW_MS) {
    if (clapCount == 2) toggle(LED1);
    if (clapCount == 3) toggle(LED2);
    clapCount = 0;
  }

  Serial.print("big:");   Serial.print(big);
  Serial.print(" small:"); Serial.print(small);
  Serial.print(" threshold:"); Serial.println(CLAP_THRESHOLD);
}
