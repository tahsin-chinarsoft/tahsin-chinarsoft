// ChinarSoft - Arduino Sensor Kit, Episode 2
// LED modules: RGB (KY-016 / KY-009), Two-color (KY-011 / KY-029), 7-color flash (KY-034)
// Project: Mood lamp with a status light
//
// Wiring (Arduino Uno) - all PWM pins except D7:
//   RGB module      R -> D9  | G -> D10 | B -> D11 | - -> GND
//   Two-color LED   R -> D5  | G -> D6  | - -> GND
//   7-color flash   S -> D7  | - -> GND
//   KY-009 and KY-011 have NO resistors on board: add 220 ohm in series on each color!
//
// Serial commands (9600 baud):
//   m = mood mode (slow rainbow)    r/g/b/w = fixed color    0 = off
//   f = toggle 7-color flash LED

const int RGB_R = 9, RGB_G = 10, RGB_B = 11;
const int BI_R = 5, BI_G = 6;
const int FLASH_PIN = 7;

const bool COMMON_ANODE = false;   // set true if your RGB module has a '+' common pin

char mode = 'm';
bool flashOn = false;
float hue = 0.0;                   // 0..360 degrees

void writeRGB(int r, int g, int b) {
  if (COMMON_ANODE) { r = 255 - r; g = 255 - g; b = 255 - b; }
  analogWrite(RGB_R, r);
  analogWrite(RGB_G, g);
  analogWrite(RGB_B, b);
}

// Convert hue (0-360) at full saturation/brightness to RGB
void hueToRGB(float h, int &r, int &g, int &b) {
  float x = 1.0 - fabs(fmod(h / 60.0, 2.0) - 1.0);
  float rf = 0, gf = 0, bf = 0;
  if (h < 60)       { rf = 1; gf = x; }
  else if (h < 120) { rf = x; gf = 1; }
  else if (h < 180) { gf = 1; bf = x; }
  else if (h < 240) { gf = x; bf = 1; }
  else if (h < 300) { rf = x; bf = 1; }
  else              { rf = 1; bf = x; }
  r = rf * 255; g = gf * 255; b = bf * 255;
}

// Two-color LED as a status light: green -> yellow -> red
void writeStatus(int level) {        // level 0..255
  analogWrite(BI_R, level);
  analogWrite(BI_G, 255 - level);
}

void handleSerial() {
  if (!Serial.available()) return;
  char c = Serial.read();
  if (c == 'f') {
    flashOn = !flashOn;
    digitalWrite(FLASH_PIN, flashOn ? HIGH : LOW);
    Serial.println(flashOn ? "7-color flash ON" : "7-color flash OFF");
  } else if (c == 'm' || c == 'r' || c == 'g' || c == 'b' || c == 'w' || c == '0') {
    mode = c;
    Serial.print("Mode: ");
    Serial.println(mode);
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(RGB_R, OUTPUT); pinMode(RGB_G, OUTPUT); pinMode(RGB_B, OUTPUT);
  pinMode(BI_R, OUTPUT);  pinMode(BI_G, OUTPUT);
  pinMode(FLASH_PIN, OUTPUT);
  Serial.println("Commands: m r g b w 0 f");
}

void loop() {
  handleSerial();

  int r = 0, g = 0, b = 0;
  switch (mode) {
    case 'm':
      hue += 0.5;
      if (hue >= 360) hue -= 360;
      hueToRGB(hue, r, g, b);
      break;
    case 'r': r = 255; break;
    case 'g': g = 255; break;
    case 'b': b = 255; break;
    case 'w': r = 255; g = 180; b = 120; break;   // white needs tuning: green and blue are brighter
    default: break;                               // '0' = off
  }
  writeRGB(r, g, b);

  // Status light follows the hue position in mood mode, otherwise shows green
  writeStatus(mode == 'm' ? (int)(hue / 360.0 * 255) : 0);

  delay(20);
}
