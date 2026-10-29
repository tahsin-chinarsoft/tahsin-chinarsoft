// ChinarSoft - Arduino Sensor Kit, Episode 9
// IR receiver (KY-022), IR transmitter (KY-005), HX1838 remote kit
// Project: IR remote cloner + remote-controlled RGB LED
//   - Point any remote at the receiver: protocol, address and command are printed
//   - Press 'l' in Serial Monitor, then a remote button: that code is LEARNED
//   - Press the kit button (or send 's') to TRANSMIT the learned code with KY-005
//   - HX1838 remote buttons 1/2/3 control the RGB LED, 0 = off
//
// Wiring (Arduino Uno):
//   KY-022 / HX1838 receiver  S -> D2 | middle -> 5V | - -> GND
//   KY-005 IR LED             S -> D3 | - -> GND   (D3 = IRremote default send pin on Uno)
//   KY-004 button             S -> D4 | middle -> 5V | - -> GND
//   RGB module                R -> D5, G -> D6, B -> D9, - -> GND
//
// Library: IRremote (Armin Joachimsmeyer) v4.x

#include <IRremote.hpp>

const int IR_RECEIVE_PIN = 2;
const int IR_SEND_PIN    = 3;
const int BUTTON_PIN     = 4;
const int RGB_R = 5, RGB_G = 6, RGB_B = 9;   // NOT 3 or 11: IRremote uses Timer2

// HX1838 remote (NEC protocol) - typical command codes, check yours in the Serial Monitor
const uint8_t CMD_0 = 0x16;
const uint8_t CMD_1 = 0x0C;
const uint8_t CMD_2 = 0x18;
const uint8_t CMD_3 = 0x5E;

IRData learned;
bool haveLearned = false;
bool learnMode = false;
int buttonIdle;

void setRGB(int r, int g, int b) {
  analogWrite(RGB_R, r); analogWrite(RGB_G, g); analogWrite(RGB_B, b);
}

void handleRemoteCommand(uint8_t cmd) {
  if (cmd == CMD_1) setRGB(255, 0, 0);
  else if (cmd == CMD_2) setRGB(0, 255, 0);
  else if (cmd == CMD_3) setRGB(0, 0, 255);
  else if (cmd == CMD_0) setRGB(0, 0, 0);
}

void sendLearned() {
  if (!haveLearned) { Serial.println("Nothing learned yet. Send 'l' first."); return; }
  IrReceiver.stop();                  // don't receive our own transmission
  IrSender.write(&learned, 2);        // send + 2 repeats
  Serial.print("Sent ");
  Serial.print(getProtocolString(learned.protocol));
  Serial.print(" cmd 0x"); Serial.println(learned.command, HEX);
  delay(100);
  IrReceiver.start();
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(RGB_R, OUTPUT); pinMode(RGB_G, OUTPUT); pinMode(RGB_B, OUTPUT);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  IrSender.begin(IR_SEND_PIN);
  delay(50);
  buttonIdle = digitalRead(BUTTON_PIN);
  Serial.println("IR cloner ready. 'l' = learn next code, 's' = send learned code");
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'l') { learnMode = true; Serial.println("Learning... press a remote button"); }
    if (c == 's') sendLearned();
  }

  static int lastButton = buttonIdle;
  int b = digitalRead(BUTTON_PIN);
  if (b != buttonIdle && lastButton == buttonIdle) { sendLearned(); delay(200); }
  lastButton = b;

  if (IrReceiver.decode()) {
    IRData &d = IrReceiver.decodedIRData;
    bool isRepeat = d.flags & IRDATA_FLAGS_IS_REPEAT;

    if (d.protocol == UNKNOWN) {
      Serial.println("Unknown protocol (raw data) - try again closer");
    } else if (!isRepeat) {
      IrReceiver.printIRResultShort(&Serial);   // protocol, address, command
      if (learnMode) {
        learned = d;
        haveLearned = true;
        learnMode = false;
        Serial.println(">> Learned!");
      } else {
        handleRemoteCommand(d.command);
      }
    }
    IrReceiver.resume();
  }
}
