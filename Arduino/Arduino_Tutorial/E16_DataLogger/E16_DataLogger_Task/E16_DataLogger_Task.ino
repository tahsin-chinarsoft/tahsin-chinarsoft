// ChinarSoft - Arduino Sensor Kit, Episode 16 - TASK SOLUTION
// Task: real timestamps - ask for the start date and time over Serial at boot, log clock times instead of seconds.
//
// What changed vs E16_DataLogger.ino:
//   - At boot the Serial Monitor asks for the start time:  2026-10-29 14:00:00   (then press Enter)
//     Set the Serial Monitor line ending to "Newline" (or "Both NL & CR"). You have 60 s.
//   - startEpoch = typed time in seconds; nowEpoch() = startEpoch + millis() / 1000.
//     Every CSV line starts with formatTime(nowEpoch()) -> "2026-10-29 14:05:10" (Excel reads it as a date).
//   - If nothing is typed within 60 s, the logger falls back to the old "seconds since boot" column.
//   - The first column is now called "time". Strings moved to flash with F() to save RAM next to the SD buffer.
//   Limits: no RTC, so the time is only as good as the Uno's crystal (a few seconds/day), it restarts on
//   every reset, and millis() wraps after 49.7 days. For long runs add a DS3231 RTC module.
//
// Wiring: identical to Episode 16, no extra parts.
//   SD module   CS -> D10 | MOSI -> D11 | MISO -> D12 | SCK -> D13 | VCC -> 5V | GND -> GND
//   DS18B20     S -> D4
//   DHT11       S -> D5
//   KY-018 LDR  S -> A0
//   KY-004 button S -> D2  (press = close the file safely before removing the card)
//   Status LED  D3 (220 ohm): blinks on every write, fast blink = error
//
// Card: FAT16/FAT32, max 32 GB, file names 8.3 format (LOG007.CSV)
// Libraries: SD (built in), OneWire, DallasTemperature, DHT sensor library

#include <SPI.h>
#include <SD.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>

const int SD_CS = 10;
const int DS_PIN = 4, DHT_PIN = 5, LDR_PIN = A0;
const int BUTTON_PIN = 2, LED_PIN = 3;
const unsigned long LOG_INTERVAL_MS = 10000;   // one line every 10 s
const int FLUSH_EVERY = 6;                     // force data to the card every 6 lines (1 min)
const unsigned long TIME_PROMPT_MS = 60000;    // TASK: how long we wait for the start time at boot

OneWire oneWire(DS_PIN);
DallasTemperature ds(&oneWire);
DHT dht(DHT_PIN, DHT11);
File logFile;
char fileName[13];
unsigned long lineCount = 0;
bool stopped = false;
int buttonIdle;

// TASK: real-time clock in software. Seconds counted from 2000-01-01 00:00:00
// (an unsigned long holds that until the year 2136).
unsigned long startEpoch;
bool timeSet = false;

unsigned long nowEpoch() {
  return startEpoch + millis() / 1000;
}

// TASK: days since 2000-01-01 for a calendar date (valid 2000..2099: every 4th year is a leap year)
unsigned long daysFromDate(int y, int m, int d) {
  static const int before[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  int yy = y - 2000;
  unsigned long days = 365UL * yy + (yy + 3) / 4;   // leap days in the years before y
  days += before[m - 1] + d - 1;
  if (m > 2 && yy % 4 == 0) days++;                  // this year's Feb 29 already passed
  return days;
}

// TASK: epoch -> "YYYY-MM-DD hh:mm:ss" (static buffer, use it right away)
const char *formatTime(unsigned long e) {
  static char buf[20];
  unsigned long days = e / 86400UL;
  unsigned long secs = e % 86400UL;
  int y = 2000;
  while (true) {
    int len = (y % 4 == 0) ? 366 : 365;
    if (days < (unsigned long)len) break;
    days -= len; y++;
  }
  static const byte mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int m = 0;
  while (true) {
    int len = mdays[m] + ((m == 1 && y % 4 == 0) ? 1 : 0);
    if (days < (unsigned long)len) break;
    days -= len; m++;
  }
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
           y, m + 1, (int)days + 1, (int)(secs / 3600), (int)(secs / 60 % 60), (int)(secs % 60));
  return buf;
}

// TASK: "2026-10-29 14:00:00" -> true + startEpoch set. Checks every field.
bool parseStartTime(const char *s) {
  int v[6], n = 0;
  long cur = -1;
  for (const char *p = s; ; p++) {
    if (*p >= '0' && *p <= '9') { cur = (cur < 0 ? 0 : cur * 10) + (*p - '0'); if (cur > 9999) return false; }
    else {                                       // any other character ends a number
      if (cur >= 0) { if (n == 6) return false; v[n++] = cur; cur = -1; }
      if (*p == 0) break;
    }
  }
  if (n != 6) return false;
  int y = v[0], mo = v[1], d = v[2], h = v[3], mi = v[4], se = v[5];
  if (y < 2000 || y > 2099 || mo < 1 || mo > 12 || d < 1 || d > 31 || h > 23 || mi > 59 || se > 59) return false;
  unsigned long typed = daysFromDate(y, mo, d) * 86400UL + h * 3600UL + mi * 60UL + se;
  startEpoch = typed - millis() / 1000;          // so nowEpoch() == typed right now
  return true;
}

// TASK: ask once at boot. Blocking is fine here: nothing is logged yet.
void askStartTime() {
  Serial.println(F("Type the start time and press Enter:  2026-10-29 14:00:00"));
  Serial.println(F("(no answer within 60 s = log seconds since boot)"));
  char line[24];
  byte len = 0;
  unsigned long t0 = millis();
  while (millis() - t0 < TIME_PROMPT_MS) {
    if (!Serial.available()) continue;
    char c = Serial.read();
    if (c == '\r') continue;
    if (c != '\n') { if (len < sizeof(line) - 1) line[len++] = c; continue; }
    line[len] = 0;
    if (len > 0 && parseStartTime(line)) {
      timeSet = true;
      Serial.print(F("Clock set: ")); Serial.println(formatTime(nowEpoch()));
      return;
    }
    Serial.println(F("Format: YYYY-MM-DD hh:mm:ss  - try again"));
    len = 0;
  }
  Serial.println(F("No time given: logging seconds since boot."));
}

void errorBlink(const __FlashStringHelper *msg) {
  Serial.println(msg);
  while (true) { digitalWrite(LED_PIN, !digitalRead(LED_PIN)); delay(100); }
}

// Find the first free name: LOG000.CSV, LOG001.CSV, ...
void openNewFile() {
  for (int i = 0; i < 1000; i++) {
    snprintf(fileName, sizeof(fileName), "LOG%03d.CSV", i);
    if (!SD.exists(fileName)) break;
  }
  logFile = SD.open(fileName, FILE_WRITE);
  if (!logFile) errorBlink(F("Could not create file"));
  logFile.println(F("time,ds18b20_C,dht11_C,humidity_pct,light_raw"));   // TASK: column renamed
  logFile.flush();
  Serial.print(F("Logging to "));
  Serial.println(fileName);
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(SD_CS, OUTPUT);                       // keep the Uno in SPI master mode
  ds.begin();
  dht.begin();
  delay(100);
  buttonIdle = digitalRead(BUTTON_PIN);

  if (!SD.begin(SD_CS)) errorBlink(F("SD init failed: card inserted? formatted FAT32? wiring?"));
  askStartTime();                               // TASK
  openNewFile();
}

void loop() {
  if (stopped) return;

  // Safe stop button
  if (digitalRead(BUTTON_PIN) != buttonIdle) {
    logFile.close();
    stopped = true;
    digitalWrite(LED_PIN, HIGH);                // steady LED = safe to remove the card
    Serial.println(F("File closed. Safe to remove the card."));
    return;
  }

  static unsigned long lastLog = 0;
  if (lastLog != 0 && millis() - lastLog < LOG_INTERVAL_MS) return;
  lastLog = millis();

  ds.requestTemperatures();
  float tDs = ds.getTempCByIndex(0);
  float tDht = dht.readTemperature();
  float hum = dht.readHumidity();
  int light = analogRead(LDR_PIN);

  // Build the CSV line. Empty field = sensor error (Excel shows a blank cell)
  if (timeSet) logFile.print(formatTime(nowEpoch()));   // TASK: real clock time
  else logFile.print(millis() / 1000);                  // fallback: seconds since boot
  logFile.print(',');
  if (tDs != DEVICE_DISCONNECTED_C) logFile.print(tDs, 2);
  logFile.print(',');
  if (!isnan(tDht)) logFile.print(tDht, 0);
  logFile.print(',');
  if (!isnan(hum)) logFile.print(hum, 0);
  logFile.print(',');
  logFile.println(light);

  lineCount++;
  if (lineCount % FLUSH_EVERY == 0) logFile.flush();   // writes the 512-byte buffer to the card

  digitalWrite(LED_PIN, HIGH); delay(30); digitalWrite(LED_PIN, LOW);
  Serial.print(F("Line ")); Serial.print(lineCount);
  if (timeSet) { Serial.print(' '); Serial.print(formatTime(nowEpoch())); }   // TASK
  Serial.print(F(": ")); Serial.print(tDs); Serial.print(F(" C, "));
  Serial.print(hum, 0); Serial.println(F(" %"));
}
