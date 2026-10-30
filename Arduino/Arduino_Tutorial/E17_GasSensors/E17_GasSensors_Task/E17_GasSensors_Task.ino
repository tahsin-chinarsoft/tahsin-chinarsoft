// ChinarSoft - Arduino Sensor Kit, Episode 17 - TASK SOLUTION
// Task: log the four gas ratios (Rs/R0) to the SD card from Episode 16, every 30 s.
//
// What changed vs E17_GasSensors.ino:
//   - Episode 16's SD logger merged in: new file LOG000.CSV, LOG001.CSV ... with the header
//       time,MQ3,MQ4,MQ7,MQ135
//     time = seconds since boot. One line every 30 s, written (flushed) to the card right away.
//   - Logging starts after the 3-minute warm-up (warm-up readings are meaningless).
//   - PIN CONFLICT FIXED: the buzzer was on D12, but D12 is MISO of the SD card -> buzzer moved to D8.
//   - Episode 16's safe-stop button (D2) and status LED (D3) are back: press = close the file.
//   - If the SD card is missing, the gas monitor and alarm keep running (only the logging is off,
//     LED blinks fast). A gas alarm must never stop because of a logging problem.
//   - The old delay(500) is gone: the Serial Plotter still gets a line every 500 ms, but the
//     buzzer pattern is now updated on every loop pass (millis() style).
//   - Strings moved to flash with F() to save RAM next to the 512-byte SD buffer.
//
// !!! These are hobby sensors. NOT a certified gas/CO detector. Never rely on them for safety. !!!
//
// POWER: each heater draws ~150 mA. Four sensors = ~600 mA -> more than USB can give.
//        Power the sensors from a separate 5 V / 2 A supply and connect its GND to Arduino GND.
//
// Wiring (Arduino Uno):
//   MQ-3   AO -> A0     MQ-4   AO -> A1
//   MQ-7   AO -> A2     MQ-135 AO -> A3
//   All VCC -> external 5 V, all GND -> external GND + Arduino GND
//   Active buzzer S -> D8                      (TASK: moved from D12)
//   SD module CS -> D10 | MOSI -> D11 | MISO -> D12 | SCK -> D13 | VCC -> 5V | GND -> GND   (TASK, as in E16)
//   KY-004 button S -> D2  (press = close the file safely)                                  (TASK, as in E16)
//   Status LED D3 (220 ohm): blinks on every write, fast blink = SD error, steady = safe to remove (TASK)
//
// Serial (115200): 'c' = calibrate in clean air (saves R0 to EEPROM)
// Output: ratio Rs/R0 per sensor. 1.00 = clean air, lower = more gas.
// EEPROM: bytes 20..35 = r0[4] (unchanged).
// Libraries: SD (built in)

#include <EEPROM.h>
#include <SPI.h>
#include <SD.h>

const int N = 4;
const char *NAMES[N]   = {"MQ3_alcohol", "MQ4_methane", "MQ7_CO", "MQ135_air"};
const int PINS[N]      = {A0, A1, A2, A3};
const float RL_KOHM[N] = {1.0, 1.0, 1.0, 1.0};   // load resistor on YOUR board (often 1k, sometimes 10k: measure it)
const float ALARM_RATIO[N] = {0.5, 0.6, 0.6, 0.6};  // alarm when Rs/R0 falls below this
const unsigned long WARMUP_MS = 3UL * 60UL * 1000UL; // 3 min minimum; 24-48 h burn-in for new sensors
const int BUZZER_PIN = 8;                        // TASK: was D12 (= SD MISO)
const int EEPROM_ADDR = 20;

// TASK: SD logger parts from Episode 16
const int SD_CS = 10;
const int BUTTON_PIN = 2, LED_PIN = 3;
const unsigned long LOG_INTERVAL_MS = 30000;     // one line every 30 s
const unsigned long PRINT_MS = 500;              // Serial Plotter rate (was delay(500))

float r0[N] = {10, 10, 10, 10};   // sensor resistance in clean air (kOhm)

File logFile;                     // TASK
char fileName[13];
bool sdOk = false, stopped = false;
int buttonIdle;
unsigned long lastLog = 0;

// Sensor resistance from the analog voltage:
// Vout = Vc * RL / (Rs + RL)  ->  Rs = RL * (Vc - Vout) / Vout
float readRs(int i) {
  long sum = 0;
  for (int k = 0; k < 32; k++) sum += analogRead(PINS[i]);
  float raw = sum / 32.0;
  if (raw < 1) raw = 1;                      // avoid divide by zero
  return RL_KOHM[i] * (1023.0 - raw) / raw;
}

void calibrate() {
  Serial.println(F("Calibrating in CLEAN AIR, 20 s..."));
  float sum[N] = {0, 0, 0, 0};
  for (int s = 0; s < 20; s++) {
    for (int i = 0; i < N; i++) sum[i] += readRs(i);
    delay(1000);
  }
  for (int i = 0; i < N; i++) r0[i] = sum[i] / 20.0;
  EEPROM.put(EEPROM_ADDR, r0);
  Serial.println(F("R0 saved."));
}

// TASK: Episode 16's file finder, with the gas header. Returns false instead of halting.
bool openNewFile() {
  for (int i = 0; i < 1000; i++) {
    snprintf(fileName, sizeof(fileName), "LOG%03d.CSV", i);
    if (!SD.exists(fileName)) break;
  }
  logFile = SD.open(fileName, FILE_WRITE);
  if (!logFile) return false;
  logFile.println(F("time,MQ3,MQ4,MQ7,MQ135"));
  logFile.flush();
  Serial.print(F("# Logging to "));
  Serial.println(fileName);
  return true;
}

// TASK: one CSV line: seconds since boot + the four ratios
void logRatios() {
  logFile.print(millis() / 1000);
  for (int i = 0; i < N; i++) {
    logFile.print(',');
    logFile.print(readRs(i) / r0[i], 2);
  }
  logFile.println();
  logFile.flush();                           // every 30 s: cheap, and a power cut loses at most one line
  digitalWrite(LED_PIN, HIGH); delay(30); digitalWrite(LED_PIN, LOW);
}

// TASK: status LED - fast blink while the SD card is not working (non-blocking)
void sdErrorBlink() {
  digitalWrite(LED_PIN, (millis() / 100) % 2);
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);                  // TASK
  pinMode(BUTTON_PIN, INPUT);
  pinMode(SD_CS, OUTPUT);                    // keep the Uno in SPI master mode
  delay(100);
  buttonIdle = digitalRead(BUTTON_PIN);

  float saved[N];
  EEPROM.get(EEPROM_ADDR, saved);
  if (!isnan(saved[0]) && saved[0] > 0.1 && saved[0] < 1000) {
    for (int i = 0; i < N; i++) r0[i] = saved[i];
    Serial.println(F("R0 loaded from EEPROM."));
  } else {
    Serial.println(F("No calibration yet: after warm-up, press 'c' in clean air."));
  }

  // TASK: SD card. No card = keep monitoring without logging.
  if (SD.begin(SD_CS) && openNewFile()) sdOk = true;
  else Serial.println(F("# SD init failed: card inserted? FAT32? wiring? Gas monitor runs WITHOUT logging."));
}

void loop() {
  // TASK: safe stop button (Episode 16)
  if (sdOk && !stopped && digitalRead(BUTTON_PIN) != buttonIdle) {
    logFile.close();
    stopped = true;
    digitalWrite(LED_PIN, HIGH);             // steady LED = safe to remove the card
    Serial.println(F("# File closed. Safe to remove the card."));
  }
  if (!sdOk) sdErrorBlink();

  // 1) Warm-up: readings are meaningless while the heater is still heating
  if (millis() < WARMUP_MS) {
    static unsigned long lastMsg = 0;
    if (millis() - lastMsg > 10000) {
      lastMsg = millis();
      Serial.print(F("# Warming up... "));
      Serial.print((WARMUP_MS - millis()) / 1000);
      Serial.println(F(" s left"));
    }
    lastLog = millis();                      // TASK: first log line 30 s after the warm-up
    return;
  }

  if (Serial.available() && Serial.read() == 'c') calibrate();

  // TASK: Episode 16's logger plus Episode 17's ratios, in one loop
  if (sdOk && !stopped && millis() - lastLog >= LOG_INTERVAL_MS) {
    lastLog = millis();
    logRatios();
  }

  // 2) Read all sensors, print ratios for the Serial Plotter (every 500 ms, no delay)
  static unsigned long lastPrint = 0;
  static bool alarm = false;
  if (millis() - lastPrint >= PRINT_MS) {
    lastPrint = millis();
    alarm = false;
    for (int i = 0; i < N; i++) {
      float ratio = readRs(i) / r0[i];
      if (ratio < ALARM_RATIO[i]) alarm = true;
      Serial.print(NAMES[i]); Serial.print(':'); Serial.print(ratio, 2);
      Serial.print(i < N - 1 ? ' ' : '\n');
    }
  }

  digitalWrite(BUZZER_PIN, alarm && (millis() / 250) % 2);   // 2 Hz pulsing alarm, updated every pass
}
