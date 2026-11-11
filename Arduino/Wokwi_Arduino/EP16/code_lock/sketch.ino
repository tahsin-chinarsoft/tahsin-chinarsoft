// Episode 16 - Keypad code lock: keypad + LCD + servo
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo lock;

const String CODE = "2580";
const int CLOSED_ANGLE = 0, OPEN_ANGLE = 90;   // "OPEN" is already taken by Keypad.h
String entered = "";
int wrong = 0;

void prompt() {
  entered = "";
  lcd.clear();
  lcd.print("Enter code:");
  lcd.setCursor(0, 1);
}

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lock.attach(10);
  lock.write(CLOSED_ANGLE);
  prompt();
}

void loop() {
  char key = keypad.getKey();      // 0 when nothing new was pressed
  if (!key) return;
  Serial.println(key);

  if (key == '*') {                // clear
    prompt();
  } else if (key == '#') {         // check
    if (entered == CODE) {
      wrong = 0;
      lcd.clear();
      lcd.print("Unlocked!");
      lock.write(OPEN_ANGLE);
      delay(5000);                 // stay open 5 s
      lock.write(CLOSED_ANGLE);
    } else {
      wrong++;
      lcd.clear();
      lcd.print("Wrong code");
      if (wrong >= 3) {
        lcd.setCursor(0, 1);
        lcd.print("Wait 10 s");
        delay(10000);              // 3 wrong tries: wait
        wrong = 0;
      } else {
        delay(1500);
      }
    }
    prompt();
  } else if (entered.length() < 4) {   // a digit or letter
    entered += key;
    lcd.print('*');                // never show the code
  }
}
