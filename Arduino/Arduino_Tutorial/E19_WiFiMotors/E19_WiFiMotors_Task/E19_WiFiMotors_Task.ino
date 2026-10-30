// ChinarSoft - Arduino Sensor Kit, Episode 19 - TASK SOLUTION
// Task: add the HC-SR04 ultrasonic sensor and refuse forward commands closer than 20 cm (automatic braking).
//
// What changed vs E19_WiFiMotors.ino:
//   - NEW readCm(): HC-SR04 distance in cm. No echo (nothing within ~4 m) counts as "far" (999).
//   - drive('F'): if (readCm() < 20) -> drive('S') and refuse. Back, left, right and stop still work,
//     so you can always reverse away from the wall.
//   - The page polls /cmd 5 times per second while you hold "forward", so the distance is checked
//     again every 200 ms while driving -> the car brakes by itself when it gets close.
//   - /cmd answers "ok" or "blocked"; the page shows "TOO CLOSE - forward blocked".
//   Watchdog, speed slider and motor pins are unchanged.
//
// Board: "NodeMCU 1.0 (ESP-12E Module)"  (Boards Manager: esp8266 by ESP8266 Community)
// Shield pins (fixed by the shield):
//   Motor A: speed D1 (GPIO5), direction D3 (GPIO0)
//   Motor B: speed D2 (GPIO4), direction D4 (GPIO2)
// TASK - HC-SR04 wiring (free pins D5/D6, not boot-strapping pins):
//   VCC  -> 5 V (shield VIN/VU pin with USB or a 5 V supply; the HC-SR04 needs 5 V)
//   GND  -> GND
//   TRIG -> D5 (GPIO14)   (3.3 V is enough to trigger it)
//   ECHO -> 1 kOhm -> D6 (GPIO12), and D6 -> 2 kOhm -> GND
//           ECHO is 5 V, NodeMCU pins take 3.3 V: 5 V x 2k / (1k + 2k) = 3.3 V
// Power: motors from VM terminal (4.5-9 V battery), keep the VIN/VM jumper as your shield's manual says.
//        Never power motors from the USB port.

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char *AP_SSID = "ChinarSoft-Motors";
const char *AP_PASS = "chinarsoft123";        // min 8 characters - change it!

const int PWM_A = D1, DIR_A = D3;
const int PWM_B = D2, DIR_B = D4;
const int TRIG_PIN = D5, ECHO_PIN = D6;       // TASK: HC-SR04 (ECHO through the 1k/2k divider)
const float STOP_CM = 20;                     // TASK: no forward closer than this
const unsigned long WATCHDOG_MS = 1000;

ESP8266WebServer server(80);
unsigned long lastCommand = 0;
int speedPct = 70;
bool blocked = false;                         // TASK: last 'F' was refused

const char PAGE[] PROGMEM = R"HTML(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ChinarSoft Motors</title>
<style>
 body{font-family:sans-serif;text-align:center;background:#111;color:#eee;margin:0;padding:16px}
 .g{display:grid;grid-template-columns:repeat(3,90px);gap:10px;justify-content:center;margin:20px auto}
 button{height:90px;font-size:28px;border:0;border-radius:14px;background:#2b6;color:#fff;touch-action:none}
 #s{background:#c33}
 input{width:80%}
</style></head><body>
<h2>ChinarSoft Motors</h2>
<div class="g">
 <span></span><button data-c="F">&#9650;</button><span></span>
 <button data-c="L">&#9664;</button><button id="s" data-c="S">&#9632;</button><button data-c="R">&#9654;</button>
 <span></span><button data-c="B">&#9660;</button><span></span>
</div>
<p>Speed: <b id="v">70</b>%</p><input type="range" id="sp" min="30" max="100" value="70">
<p id="st">connected</p>
<script>
let cmd='S';
document.querySelectorAll('button').forEach(b=>{
  b.onpointerdown=()=>{cmd=b.dataset.c;};
  b.onpointerup=b.onpointerleave=()=>{cmd='S';};
});
sp.oninput=()=>{v.textContent=sp.value;};
// Send the current command 5 times per second. If this stops, the car stops.
setInterval(()=>{
  fetch('/cmd?c='+cmd+'&s='+sp.value).then(r=>r.text())
  .then(t=>st.textContent=(t=='blocked')?'TOO CLOSE - forward blocked':'connected')
  .catch(()=>st.textContent='NO CONNECTION');
},200);
</script></body></html>
)HTML";

// TASK: HC-SR04 distance in cm. 25 ms timeout = about 4 m; no echo = nothing in front = 999
float readCm() {
  digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long us = pulseIn(ECHO_PIN, HIGH, 25000UL);
  if (us == 0) return 999;
  return us * 0.0343 / 2.0;
}

// dir: +1 forward, -1 backward, 0 stop. Speed 0..100 %
void motor(int pwmPin, int dirPin, int dir, int pct) {
  digitalWrite(dirPin, dir < 0 ? HIGH : LOW);
  analogWrite(pwmPin, dir == 0 ? 0 : map(pct, 0, 100, 0, 1023));
}

void drive(char c) {
  blocked = false;
  switch (c) {
    case 'F':
      if (readCm() < STOP_CM) {               // TASK: automatic braking
        drive('S');                           // too close: refuse
        blocked = true;
        break;
      }
      motor(PWM_A, DIR_A,  1, speedPct); motor(PWM_B, DIR_B,  1, speedPct); break;
    case 'B': motor(PWM_A, DIR_A, -1, speedPct); motor(PWM_B, DIR_B, -1, speedPct); break;
    case 'L': motor(PWM_A, DIR_A, -1, speedPct); motor(PWM_B, DIR_B,  1, speedPct); break;  // spin in place
    case 'R': motor(PWM_A, DIR_A,  1, speedPct); motor(PWM_B, DIR_B, -1, speedPct); break;
    default:  motor(PWM_A, DIR_A,  0, 0);        motor(PWM_B, DIR_B,  0, 0);        break;
  }
}

void handleCmd() {
  if (server.hasArg("s")) speedPct = constrain(server.arg("s").toInt(), 0, 100);
  char c = server.hasArg("c") ? server.arg("c")[0] : 'S';
  drive(c);
  lastCommand = millis();
  server.send(200, "text/plain", blocked ? "blocked" : "ok");   // TASK: tell the page
}

void setup() {
  Serial.begin(115200);
  pinMode(PWM_A, OUTPUT); pinMode(DIR_A, OUTPUT);
  pinMode(PWM_B, OUTPUT); pinMode(DIR_B, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);                    // TASK
  pinMode(ECHO_PIN, INPUT);                     // TASK
  digitalWrite(TRIG_PIN, LOW);
  analogWriteRange(1023);                       // core 3.x defaults to 255, make it explicit
  analogWriteFreq(1000);
  drive('S');

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("\nConnect to WiFi '"); Serial.print(AP_SSID);
  Serial.print("' and open http://"); Serial.println(WiFi.softAPIP());

  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.on("/cmd", handleCmd);
  server.begin();
}

void loop() {
  server.handleClient();
  if (millis() - lastCommand > WATCHDOG_MS) drive('S');   // lost connection = stop
}
