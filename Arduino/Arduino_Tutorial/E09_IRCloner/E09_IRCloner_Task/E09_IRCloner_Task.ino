// ChinarSoft - Arduino Sensor Kit, Episode 9 - TASK SOLUTION
// Task: learn three different IR codes and send them from three different inputs.
//
// What changed vs E09_IRCloner:
//   - IRData learned  ->  IRData learned[3]  plus  int slot.
//   - 'l' learns the next remote button into learned[slot], then slot = (slot + 1) % 3,
//     so three 'l' presses fill slots 0, 1, 2 (a fourth overwrites slot 0 again).
//   - Three inputs, each sends its own slot:  input 0 = KY-004 button on D4 (as before),
//     input 1 = push button on D7, input 2 = push button on D8.
//     From the Serial Monitor: '0', '1', '2' send that slot, 's' still sends slot 0.
//   - Buttons now use a millis() debounce instead of delay(200).
//   - Empty slots are detected by protocol == UNKNOWN (global array starts zeroed).
//   - HX1838 remote -> RGB LED control unchanged.
//
// Wiring: same wiring as E09_IRCloner, plus TWO extra push buttons:
//   KY-022 / HX1838 receiver  S -> D2 | middle -> 5V | - -> GND
//   KY-005 IR LED             S -> D3 | - -> GND   (D3 = IRremote default send pin on Uno)
//   KY-004 button (input 0)   S -> D4 | middle -> 5V | - -> GND
//   RGB module                R -> D5, G -> D6, B -> D9, - -> GND
//   TASK: button input 1      D7 -> button -> GND   (internal pull-up, no resistor needed)
//   TASK: button input 2      D8 -> button -> GND   (internal pull-up, no resistor needed)
//   (D7/D8 are plain digital pins: no conflict with IRremote's Timer2 on D3/D11.)
//
// Library: IRremote (Armin Joachimsmeyer) v4.x

#include <IRremote.hpp>

const int IR_RECEIVE_PIN = 2;
const int IR_SEND_PIN    = 3;
const int RGB_R = 5, RGB_G = 6, RGB_B = 9;   // NOT 3 or 11: IRremote uses Timer2

// TASK: three inputs, input i sends learned[i]
const int NUM_SLOTS = 3;
const int INPUT_PINS[NUM_SLOTS] = {4, 7, 8};
const unsigned long DEBOUNCE_MS = 50;

// HX1838 remote (NEC protocol) - typical command codes, check yours in the Serial Monitor
const uint8_t CMD_0 = 0x16;
const uint8_t CMD_1 = 0x0C;
const uint8_t CMD_2 = 0x18;
const uint8_t CMD_3 = 0x5E;

IRData learned[NUM_SLOTS];      // TASK: an array instead of one variable
int slot = 0;                   // TASK: next slot to learn into
bool learnMode = false;

int idleState[NUM_SLOTS];       // TASK: per-input idle level (KY-004 may idle HIGH or LOW)
int lastState[NUM_SLOTS];
unsigned long lastChange[NUM_SLOTS];

void setRGB(int r, int g, int b) {
  analogWrite(RGB_R, r); analogWrite(RGB_G, g); analogWrite(RGB_B, b);
}

void handleRemoteCommand(uint8_t cmd) {
  if (cmd == CMD_1) setRGB(255, 0, 0);
  else if (cmd == CMD_2) setRGB(0, 255, 0);
  else if (cmd == CMD_3) setRGB(0, 0, 255);
  else if (cmd == CMD_0) setRGB(0, 0, 0);
}

// TASK: send one learned code (was sendLearned())
void send(IRData &d) {
  if (d.protocol == UNKNOWN) { Serial.println("That slot is empty. Send 'l' first."); return; }
  IrReceiver.stop();                  // don't receive our own transmission
  IrSender.write(&d, 2);              // send + 2 repeats
  Serial.print("Sent ");
  Serial.print(getProtocolString(d.protocol));
  Serial.print(" cmd 0x"); Serial.println(d.command, HEX);
  delay(100);
  IrReceiver.start();
}

// TASK: debounced "just pressed" edge for input i
bool pressed(int i) {
  int s = digitalRead(INPUT_PINS[i]);
  if (s != lastState[i] && millis() - lastChange[i] > DEBOUNCE_MS) {
    lastChange[i] = millis();
    lastState[i] = s;
    return s != idleState[i];         // left the idle level = pressed
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  pinMode(INPUT_PINS[0], INPUT);          // KY-004 has its own resistor, as in the base sketch
  pinMode(INPUT_PINS[1], INPUT_PULLUP);   // TASK
  pinMode(INPUT_PINS[2], INPUT_PULLUP);   // TASK
  pinMode(RGB_R, OUTPUT); pinMode(RGB_G, OUTPUT); pinMode(RGB_B, OUTPUT);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  IrSender.begin(IR_SEND_PIN);
  delay(50);
  for (int i = 0; i < NUM_SLOTS; i++) {   // TASK: remember each input's idle level
    idleState[i] = digitalRead(INPUT_PINS[i]);
    lastState[i] = idleState[i];
    lastChange[i] = millis();
  }
  Serial.println("IR cloner ready. 'l' = learn into next slot, '0'/'1'/'2' (or buttons) = send slot");
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'l') {
      learnMode = true;
      Serial.print("Learning into slot "); Serial.print(slot);
      Serial.println("... press a remote button");
    }
    if (c == 's') send(learned[0]);                       // old command: slot 0
    if (c >= '0' && c <= '2') send(learned[c - '0']);     // TASK
  }

  // TASK: inputs 0, 1, 2 send their slot
  for (int i = 0; i < NUM_SLOTS; i++) {
    if (pressed(i)) send(learned[i]);
  }

  if (IrReceiver.decode()) {
    IRData &d = IrReceiver.decodedIRData;
    bool isRepeat = d.flags & IRDATA_FLAGS_IS_REPEAT;

    if (d.protocol == UNKNOWN) {
      Serial.println("Unknown protocol (raw data) - try again closer");
    } else if (!isRepeat) {
      IrReceiver.printIRResultShort(&Serial);   // protocol, address, command
      if (learnMode) {
        learned[slot] = d;                      // TASK: learn into the current slot...
        Serial.print(">> Learned into slot "); Serial.println(slot);
        slot = (slot + 1) % NUM_SLOTS;          // TASK: ...then move to the next one
        learnMode = false;
      } else {
        handleRemoteCommand(d.command);
      }
    }
    IrReceiver.resume();
  }
}
