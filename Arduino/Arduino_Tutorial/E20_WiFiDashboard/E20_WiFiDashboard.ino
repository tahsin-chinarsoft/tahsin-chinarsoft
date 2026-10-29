// ChinarSoft - Arduino Sensor Kit, Episode 20 (Series finale)
// NodeMCU (ESP8266) WiFi sensor dashboard
//   - DS18B20 temperature, MQ-135 air quality, reed-switch door state
//   - Joins your home WiFi; open http://chinarsoft.local (or the IP from Serial Monitor)
//   - Live page with values + 10-minute temperature chart, JSON API at /data
//   - If home WiFi is not available, it starts its own network "ChinarSoft-Dashboard"
//
// Board: "NodeMCU 1.0 (ESP-12E Module)"
// Wiring:
//   DS18B20 (KY-001)  S -> D5 (GPIO14) | middle -> 3V3 | - -> GND
//   MQ-135 AO -> 10k -> A0, and A0 -> 20k -> GND   (divider: MQ outputs up to 5 V, NodeMCU A0 max 3.3 V)
//            MQ-135 VCC -> 5 V supply (VIN/USB 5V), GND -> GND
//   KY-021 reed       S -> D6 (GPIO12) | - -> GND   (internal pull-up used)
// Libraries: OneWire, DallasTemperature (+ ESP8266 core libraries)

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const char *WIFI_SSID = "YOUR_WIFI";          // <-- your network
const char *WIFI_PASS = "YOUR_PASSWORD";
const char *AP_SSID   = "ChinarSoft-Dashboard";
const char *AP_PASS   = "chinarsoft123";

const int DS_PIN = D5;
const int REED_PIN = D6;
const int MQ_PIN = A0;

const int HISTORY = 120;                      // 120 points x 5 s = 10 minutes
const unsigned long SAMPLE_MS = 5000;

OneWire oneWire(DS_PIN);
DallasTemperature ds(&oneWire);
ESP8266WebServer server(80);

float tempHist[HISTORY];
int histHead = 0, histCount = 0;
float tempC = NAN;
int airRaw = 0;
bool doorOpen = false;
unsigned long doorOpenings = 0;

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
  j += ",\"history\":[";
  for (int i = 0; i < histCount; i++) {
    int idx = (histHead - histCount + i + HISTORY) % HISTORY;   // oldest first
    if (i) j += ',';
    j += isnan(tempHist[idx]) ? String("null") : String(tempHist[idx], 2);
  }
  j += "]}";
  server.send(200, "application/json", j);
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to "); Serial.print(WIFI_SSID);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) { delay(500); Serial.print('.'); }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nConnected. Open http://"); Serial.println(WiFi.localIP());
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.print("\nHome WiFi not found. Join '"); Serial.print(AP_SSID);
    Serial.print("' and open http://"); Serial.println(WiFi.softAPIP());
  }
  if (MDNS.begin("chinarsoft")) Serial.println("Also at http://chinarsoft.local");
}

void sample() {
  float t = ds.getTempCByIndex(0);
  tempC = (t == DEVICE_DISCONNECTED_C) ? NAN : t;
  ds.requestTemperatures();                    // non-blocking, ready by next sample
  airRaw = analogRead(MQ_PIN);

  tempHist[histHead] = tempC;
  histHead = (histHead + 1) % HISTORY;
  if (histCount < HISTORY) histCount++;
}

void setup() {
  Serial.begin(115200);
  pinMode(REED_PIN, INPUT_PULLUP);
  ds.begin();
  ds.setWaitForConversion(false);
  ds.requestTemperatures();

  connectWiFi();
  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.on("/data", handleData);
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
}
