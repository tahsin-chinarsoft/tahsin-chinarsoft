// ChinarSoft - Arduino Sensor Kit, Episode 16
// SD card module + DS18B20 + DHT11 + photoresistor
// Project: Environmental data logger -> CSV file you can open in Excel
//
// Wiring (Arduino Uno):
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

OneWire oneWire(DS_PIN);
DallasTemperature ds(&oneWire);
DHT dht(DHT_PIN, DHT11);
File logFile;
char fileName[13];
unsigned long lineCount = 0;
bool stopped = false;
int buttonIdle;

void errorBlink(const char *msg) {
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
  if (!logFile) errorBlink("Could not create file");
  logFile.println("time_s,ds18b20_C,dht11_C,humidity_pct,light_raw");
  logFile.flush();
  Serial.print("Logging to ");
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

  if (!SD.begin(SD_CS)) errorBlink("SD init failed: card inserted? formatted FAT32? wiring?");
  openNewFile();
}

void loop() {
  if (stopped) return;

  // Safe stop button
  if (digitalRead(BUTTON_PIN) != buttonIdle) {
    logFile.close();
    stopped = true;
    digitalWrite(LED_PIN, HIGH);                // steady LED = safe to remove the card
    Serial.println("File closed. Safe to remove the card.");
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
  logFile.print(millis() / 1000);
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
  Serial.print("Line "); Serial.print(lineCount);
  Serial.print(": "); Serial.print(tDs); Serial.print(" C, ");
  Serial.print(hum, 0); Serial.println(" %");
}
