// ChinarSoft - Arduino Sensor Kit, Episode 20 (Series finale) - TASK SOLUTION
// Task: send the dashboard values to the SD logger from Episode 16 (or to a cloud service of your choice).
//
// What changed vs E20_WiFiDashboard.ino:
//   - SD card logging on the NodeMCU (SD.h from the ESP8266 core). Every 5 s sample appends one CSV line
//     to DASH.CSV with the same values the dashboard shows:
//       datetime,uptime_s,temp_C,air_raw,door_open,door_openings
//     The file is opened, appended and closed every time, so you can pull the card at any moment
//     (at most the current line is lost) - no "safe stop" button needed.
//   - datetime comes from the internet (NTP) when the NodeMCU is on your home WiFi; in hotspot mode
//     that column stays empty and uptime_s is your time axis.
//   - No SD card / card pulled out: the dashboard keeps working, the page shows "SD: no card",
//     and the sketch tries to mount the card again every 10 s (insert it and logging resumes).
//   - New: http://chinarsoft.local/log downloads DASH.CSV straight from the card.
//   - Optional cloud upload to ThingSpeak: set USE_THINGSPEAK to 1 and paste your Write API key.
//     Disabled by default - SD is the working default.
//   - PIN MOVES: SD uses the hardware SPI pins D5/D6/D7, which were DS18B20 and reed in Episode 20.
//     DS18B20 moved D5 -> D2, reed moved D6 -> D1.
//
// Board: "NodeMCU 1.0 (ESP-12E Module)"
// Wiring:
//   DS18B20 (KY-001)  S -> D2 (GPIO4) | middle -> 3V3 | - -> GND          (TASK: was D5)
//   KY-021 reed       S -> D1 (GPIO5) | - -> GND   (internal pull-up used)  (TASK: was D6)
//   MQ-135 AO -> 10k -> A0, and A0 -> 20k -> GND   (divider: MQ outputs up to 5 V, NodeMCU A0 max 3.3 V)
//            MQ-135 VCC -> 5 V supply (VIN/USB 5V), GND -> GND
//   TASK - SD card module (SPI):
//     CS -> D8 (GPIO15) | MOSI -> D7 (GPIO13) | MISO -> D6 (GPIO12) | SCK -> D5 (GPIO14)
//     Module with regulator + level shifter (the usual Uno one): VCC -> VIN/VU (5 V), GND -> GND
//     Bare 3.3 V microSD breakout: VCC -> 3V3.
//     D8 must be LOW at boot (the NodeMCU has a pull-down on it). If your module pulls CS high and the
//     board no longer boots, move CS to D0 (GPIO16) and change SD_CS below.
//   Card: FAT16/FAT32.
// Libraries: OneWire, DallasTemperature (+ ESP8266 core libraries: SD, SPI, ESP8266HTTPClient)

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>                               // TASK
#include <SD.h>                                // TASK: the ESP8266 core's SD library
#include <time.h>                              // TASK: NTP clock

// TASK: optional cloud upload. 0 = off (default), 1 = also send to ThingSpeak every 20 s
#define USE_THINGSPEAK 0
#if USE_THINGSPEAK
#include <ESP8266HTTPClient.h>
const char *TS_API_KEY = "YOUR_WRITE_API_KEY";            // ThingSpeak channel -> API Keys -> Write API Key
const unsigned long TS_INTERVAL_MS = 20000;              // free accounts: max one update per 15 s
#endif

const char *WIFI_SSID = "YOUR_WIFI";          // <-- your network
const char *WIFI_PASS = "YOUR_PASSWORD";
const char *AP_SSID   = "ChinarSoft-Dashboard";
const char *AP_PASS   = "chinarsoft123";
const char *TZ_INFO   = "<+03>-3";            // TASK: your time zone (POSIX). Turkey = "<+03>-3",
                                              // UK = "GMT0BST,M3.5.0/1,M10.5.0", CET = "CET-1CEST,M3.5.0,M10.5.0/3"

const int DS_PIN = D2;                        // TASK: was D5 (now SD SCK)
const int REED_PIN = D1;                      // TASK: was D6 (now SD MISO)
const int MQ_PIN = A0;
const int SD_CS = D8;                         // TASK
const char *LOG_FILE = "/DASH.CSV";           // TASK

const int HISTORY = 120;                      // 120 points x 5 s = 10 minutes
const unsigned long SAMPLE_MS = 5000;
const unsigned long SD_RETRY_MS = 10000;      // TASK: try to mount a missing card every 10 s

OneWire oneWire(DS_PIN);
DallasTemperature ds(&oneWire);
ESP8266WebServer server(80);

float tempHist[HISTORY];
int histHead = 0, histCount = 0;
float tempC = NAN;
int airRaw = 0;
bool doorOpen = false;
unsigned long doorOpenings = 0;

bool sdOk = false;                            // TASK: card mounted and last write worked
unsigned long sdLines = 0;                    // TASK: lines written since boot
unsigned long lastSdTry = 0;

const char PAGE[] PROGMEM = R"HTML(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ChinarSoft Dashboard</title>
<style>
 body{font-family:sans-serif;background:#0f1115;color:#e8e8e8;margin:0;padding:16px}
 h2{margin:0 0 12px}
 .cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:12px}
 .c{background:#1b1f27;border-radius:12px;padding:14px}
 .l{font-size:13px;color:#9aa}.v{font-size:30px;font-weight:bold;margin-top:4px}
 canvas{width:100%;height:180px;background:#1b1f27;border-radius:12px;margin-top:12px}
 .open{color:#ff6b5b}.closed{color:#5bd68a}
 a{color:#4ea1ff}
</style></head><body>
<h2>ChinarSoft Dashboard</h2>
<div class="cards">
 <div class="c"><div class="l">Temperature</div><div class="v" id="t">--</div></div>
 <div class="c"><div class="l">Air quality (raw, lower = cleaner)</div><div class="v" id="a">--</div></div>
 <div class="c"><div class="l">Door</div><div class="v" id="d">--</div></div>
 <div class="c"><div class="l">Door openings</div><div class="v" id="n">--</div></div>
</div>
<canvas id="ch" width="600" height="180"></canvas>
<p class="l" id="u">waiting for data...</p>
<p class="l"><span id="sd">SD: --</span> · <a href="/log">download DASH.CSV</a></p>
<script>
function draw(h){
  const c=ch.getContext('2d'),W=ch.width,H=ch.height;c.clearRect(0,0,W,H);
  const v=h.filter(x=>x!==null);if(v.length<2)return;
  let lo=Math.min(...v)-0.5,hi=Math.max(...v)+0.5;
  c.strokeStyle='#4ea1ff';c.lineWidth=2;c.beginPath();
  h.forEach((x,i)=>{if(x===null)return;const X=i*W/(h.length-1),Y=H-(x-lo)/(hi-lo)*H;i?c.lineTo(X,Y):c.moveTo(X,Y);});
  c.stroke();c.fillStyle='#9aa';c.font='12px sans-serif';
  c.fillText(hi.toFixed(1)+' C',4,14);c.fillText(lo.toFixed(1)+' C',4,H-4);c.fillText('last 10 min',W-80,14);
}
async function upd(){
  try{
    const r=await fetch('/data');const j=await r.json();
    t.textContent=j.temp===null?'ERR':j.temp.toFixed(1)+' °C';
    a.textContent=j.air;
    d.textContent=j.door?'OPEN':'closed';d.className='v '+(j.door?'open':'closed');
    n.textContent=j.openings;draw(j.history);
    u.textContent='updated '+new Date().toLocaleTimeString()+' · uptime '+j.uptime+' s';
    sd.textContent=j.sd?('SD: logging, '+j.lines+' lines'):'SD: no card - not logging';
  }catch(e){u.textContent='NO CONNECTION';}
}
upd();setInterval(upd,2000);
</script></body></html>
)HTML";

void handleData() {
  // Build JSON by hand: small and no extra library
  String j = "{\"temp\":";
  j += isnan(tempC) ? String("null") : String(tempC, 2);
  j += ",\"air\":" + String(airRaw);
  j += ",\"door\":" + String(doorOpen ? "true" : "false");
  j += ",\"openings\":" + String(doorOpenings);
  j += ",\"uptime\":" + String(millis() / 1000);
  j += ",\"sd\":" + String(sdOk ? "true" : "false");      // TASK
  j += ",\"lines\":" + String(sdLines);                   // TASK
  j += ",\"history\":[";
  for (int i = 0; i < histCount; i++) {
    int idx = (histHead - histCount + i + HISTORY) % HISTORY;   // oldest first
    if (i) j += ',';
    j += isnan(tempHist[idx]) ? String("null") : String(tempHist[idx], 2);
  }
  j += "]}";
  server.send(200, "application/json", j);
}

// TASK: download the log file from the card
void handleLog() {
  File f = sdOk ? SD.open(LOG_FILE, FILE_READ) : File();
  if (!f) { server.send(404, "text/plain", "No SD card or no log file yet"); return; }
  server.sendHeader("Content-Disposition", "attachment; filename=DASH.CSV");
  server.streamFile(f, "text/csv");
  f.close();
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to "); Serial.print(WIFI_SSID);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) { delay(500); Serial.print('.'); }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nConnected. Open http://"); Serial.println(WiFi.localIP());
    configTime(TZ_INFO, "pool.ntp.org", "time.nist.gov");   // TASK: real clock for the CSV (runs in the background)
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.print("\nHome WiFi not found. Join '"); Serial.print(AP_SSID);
    Serial.print("' and open http://"); Serial.println(WiFi.softAPIP());
  }
  if (MDNS.begin("chinarsoft")) Serial.println("Also at http://chinarsoft.local");
}

// TASK: mount the card and write the CSV header if the file is new
bool sdMount() {
  SD.end(false);                              // forget a previous (failed / pulled) mount, keep SPI running
  if (!SD.begin(SD_CS)) return false;
  if (!SD.exists(LOG_FILE)) {
    File f = SD.open(LOG_FILE, FILE_WRITE);
    if (!f) return false;
    f.println("datetime,uptime_s,temp_C,air_raw,door_open,door_openings");
    f.close();
  }
  Serial.print("SD card OK, logging to "); Serial.println(LOG_FILE);
  return true;
}

// TASK: "2026-10-30 14:05:10" if NTP has answered, else "" (empty CSV field)
String dateTimeNow() {
  time_t now = time(nullptr);
  if (now < 1700000000) return "";            // clock not set yet (before Nov 2023 = not synced)
  char buf[20];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
  return String(buf);
}

// TASK: append one CSV line with the same values the dashboard shows
void logToSd() {
  if (!sdOk) return;
  File f = SD.open(LOG_FILE, FILE_WRITE);     // FILE_WRITE appends on the ESP8266 core
  String line = dateTimeNow();
  line += ',' + String(millis() / 1000);
  line += ',';
  if (!isnan(tempC)) line += String(tempC, 2);   // empty field = sensor error (like Episode 16)
  line += ',' + String(airRaw);
  line += ',' + String(doorOpen ? 1 : 0);
  line += ',' + String(doorOpenings);
  size_t written = f ? f.println(line) : 0;
  if (f) f.close();
  if (written == 0) {                         // card pulled out or full
    sdOk = false;
    lastSdTry = millis();
    Serial.println("SD write failed - dashboard keeps running, retrying the card every 10 s");
    return;
  }
  sdLines++;
}

#if USE_THINGSPEAK
// TASK (optional): ThingSpeak fields 1..4 = temperature, air, door, openings. Needs home WiFi.
void uploadThingSpeak() {
  if (WiFi.status() != WL_CONNECTED) return;
  WiFiClient client;
  HTTPClient http;
  String url = "http://api.thingspeak.com/update?api_key=" + String(TS_API_KEY);
  if (!isnan(tempC)) url += "&field1=" + String(tempC, 2);
  url += "&field2=" + String(airRaw);
  url += "&field3=" + String(doorOpen ? 1 : 0);
  url += "&field4=" + String(doorOpenings);
  http.setTimeout(3000);                      // never block the dashboard for long
  if (http.begin(client, url)) {
    int code = http.GET();
    Serial.print("ThingSpeak: HTTP "); Serial.println(code);   // 200 + entry number = OK, "0" = rejected
    http.end();
  }
}
#endif

void sample() {
  float t = ds.getTempCByIndex(0);
  tempC = (t == DEVICE_DISCONNECTED_C) ? NAN : t;
  ds.requestTemperatures();                    // non-blocking, ready by next sample
  airRaw = analogRead(MQ_PIN);

  tempHist[histHead] = tempC;
  histHead = (histHead + 1) % HISTORY;
  if (histCount < HISTORY) histCount++;

  logToSd();                                   // TASK
}

void setup() {
  Serial.begin(115200);
  pinMode(REED_PIN, INPUT_PULLUP);
  ds.begin();
  ds.setWaitForConversion(false);
  ds.requestTemperatures();

  // TASK: SD card. No card = dashboard only, retry later.
  sdOk = sdMount();
  if (!sdOk) Serial.println("No SD card (or wiring/format problem) - dashboard runs without logging");
  lastSdTry = millis();

  connectWiFi();
  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.on("/data", handleData);
  server.on("/log", handleLog);                // TASK
  server.begin();
  MDNS.addService("http", "tcp", 80);
}

void loop() {
  server.handleClient();
  MDNS.update();

  // Door: magnet on the door closes the reed -> pin LOW = closed
  // Accept a change only after it has been stable for 50 ms (reed contacts bounce)
  static bool lastRead = false;
  static unsigned long changedAt = 0;
  bool open = digitalRead(REED_PIN) == HIGH;
  if (open != lastRead) { lastRead = open; changedAt = millis(); }
  if (open != doorOpen && millis() - changedAt > 50) {
    if (open) doorOpenings++;
    doorOpen = open;
  }

  static unsigned long lastSample = 0;
  if (millis() - lastSample >= SAMPLE_MS) { lastSample = millis(); sample(); }

  // TASK: card missing? try again every 10 s
  if (!sdOk && millis() - lastSdTry >= SD_RETRY_MS) {
    lastSdTry = millis();
    sdOk = sdMount();
  }

#if USE_THINGSPEAK
  static unsigned long lastTs = 0;
  if (millis() - lastTs >= TS_INTERVAL_MS) { lastTs = millis(); uploadThingSpeak(); }
#endif
}
