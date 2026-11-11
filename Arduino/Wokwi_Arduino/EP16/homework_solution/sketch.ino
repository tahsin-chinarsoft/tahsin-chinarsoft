// Episode 16 homework solution - code lock: change the code with A, countdown during the lockout
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

char keys[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[4] = {9, 8, 7, 6};
byte colPins[4] = {5, 4, 3, 2};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, 4, 4);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo lock;

const int CLOSED_ANGLE = 0, OPEN_ANGLE = 90;
String code = "2580";                       // no longer const: it can change
String entered = "";
int wrong = 0;
enum Mode { NORMAL, OLD_CODE, NEW_CODE };
Mode mode = NORMAL;
unsigned long lockUntil = 0;                // lockout end time (millis)
int shownSeconds = -1;

void prompt() {
  entered = "";
  lcd.clear();
  if (mode == NORMAL) lcd.print("Enter code:");
  else if (mode == OLD_CODE) lcd.print("Old code:");
  else lcd.print("New code:");
  lcd.setCursor(0, 1);
}

void message(const char* text, unsigned long ms) {
  lcd.clear();
  lcd.print(text);
  delay(ms);
}

void wrongCode() {
  wrong++;
  if (wrong >= 3) {
    wrong = 0;
    lockUntil = millis() + 10000;           // 10 s lockout, counted with millis
    shownSeconds = -1;
    lcd.clear();
    lcd.print("Wrong code");
  } else {
    message("Wrong code", 1500);
    prompt();
  }
}

void setup() {
  lcd.init();
  lcd.backlight();
  lock.attach(10);
  lock.write(CLOSED_ANGLE);
  prompt();
}

void loop() {
  char key = keypad.getKey();

  // lockout: show a countdown, ignore keys
  if (lockUntil != 0) {
    if (millis() >= lockUntil) {
      lockUntil = 0;
      mode = NORMAL;
      prompt();
    } else {
      int left = (lockUntil - millis() + 999) / 1000;
      if (left != shownSeconds) {
        shownSeconds = left;
        lcd.setCursor(0, 1);
        lcd.print("Wait ");
        lcd.print(left);
        lcd.print(" s   ");
      }
    }
    return;
  }

  if (!key) return;

  if (key == '*') {                         // clear, and leave change mode
    mode = NORMAL;
    prompt();
  } else if (key == 'A' && mode == NORMAL) {  // start changing the code
    mode = OLD_CODE;
    prompt();
  } else if (key == '#') {
    if (mode == NORMAL) {
      if (entered == code) {
        wrong = 0;
        lcd.clear();
        lcd.print("Unlocked!");
        lock.write(OPEN_ANGLE);
        delay(5000);
        lock.write(CLOSED_ANGLE);
        prompt();
      } else {
        wrongCode();
      }
    } else if (mode == OLD_CODE) {
      if (entered == code) {
        wrong = 0;
        mode = NEW_CODE;
        prompt();
      } else {
        mode = NORMAL;
        wrongCode();
      }
    } else if (mode == NEW_CODE) {
      if (entered.length() == 4) {
        code = entered;
        mode = NORMAL;
        message("Code changed", 1500);
      } else {
        message("Need 4 digits", 1500);
      }
      prompt();
    }
  } else if (entered.length() < 4 && key >= '0' && key <= '9') {
    entered += key;
    lcd.print('*');
  }
}
