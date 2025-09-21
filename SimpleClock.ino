#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

void printPartOfClock(LiquidCrystal_I2C & lcd, uint8_t partOfClock) {
    if (partOfClock < 10) {
      lcd.print('0');
      lcd.print(partOfClock);
    }
    else
      lcd.print(partOfClock);
}

void playClock(LiquidCrystal_I2C & lcd, uint8_t & hour, uint8_t & min, uint8_t & sec)
{
  static unsigned long start = millis();
  unsigned long timeNow = millis();

  static const int ONE_SECOND = 1000; 
  if (timeNow - start >= ONE_SECOND) {
    start += ONE_SECOND;
    
    sec++;
    if (sec >= 60) {
      sec = 0;
      min++;
    }

    if (min >= 60) {
      min = 0;
      hour++;
    }

    if (hour >= 24)
      hour = 0;
    
    // print clock
    lcd.setCursor(4, 0);
    printPartOfClock(lcd, hour);
    lcd.print(':');
    printPartOfClock(lcd, min);
    lcd.print(':');
    printPartOfClock(lcd, sec);
  }
}

bool readButton(uint8_t btnPin, unsigned long & lastPressTime, unsigned long debounceDelay = 150) {
    uint8_t reading = digitalRead(btnPin);

    if (reading == LOW && millis() - lastPressTime > debounceDelay) {
        lastPressTime = millis();
        return true;  // button was pressed once
    }
    return false;  // no new press
}

void splitNumberToBuffer(uint8_t number, char buffer[]) {
  buffer[0] = '0' + (number / 10);
  buffer[1] = '0' + (number % 10);
}

uint8_t parseUInt8(char buffer[]) {
  return (buffer[0] - '0') * 10 + (buffer[1] - '0');
}

int main()
{
  init();
  Serial.begin(9600);

  const uint8_t INCREMENT_BTN_PIN = 12;
  const uint8_t NEXT_PART_BTN_PIN = 13;

  // setup buttons
  pinMode(INCREMENT_BTN_PIN, INPUT_PULLUP);
  pinMode(NEXT_PART_BTN_PIN, INPUT_PULLUP);

  // setup lcd display
  LiquidCrystal_I2C lcd(0x27, 16, 2);
  lcd.init();
  lcd.backlight();

  // the 3 positions we are going to print at
  const uint8_t printPositions[3] = {4, 7, 10};

  // print the initial state of the clock
  lcd.setCursor(printPositions[0], 0);
  lcd.print("00:00:00");

  // print the initial state of the indicator
  lcd.setCursor(printPositions[0], 1);
  lcd.print("^^");

  // the clock -> 00:00:00
  char clock[3][2] = {
    {'0', '0'},
    {'0', '0'},
    {'0', '0'},
  };

  // variables for debounce
  unsigned long lastIncrementTime = 0;
  unsigned long lastNextPartTraversalTime = 0;

  uint8_t clockIdx = 0; // 0 -> hours, 1 -> minutes, 2 -> seconds
  int hours = 0, minutes = 0, seconds = 0;

  while (true) {
    bool incrementBtnReading = readButton(INCREMENT_BTN_PIN, lastIncrementTime);
    bool nextPartPressed = readButton(NEXT_PART_BTN_PIN, lastNextPartTraversalTime);

    // btn increment was pressed
    if (incrementBtnReading) {
      switch (clockIdx) {
        case 0:
          hours = (hours + 1) % 24;
          splitNumberToBuffer(hours, clock[0]);
          break;
        case 1:
          minutes = (minutes + 1) % 60;
          splitNumberToBuffer(minutes, clock[1]);
          break;
        case 2:
          seconds = (seconds + 1) % 60;
          splitNumberToBuffer(seconds, clock[2]);
          break;
      }
      // print the new state of the numbers
      lcd.setCursor(printPositions[clockIdx], 0);
      lcd.print(clock[clockIdx][0]); // print tens
      lcd.print(clock[clockIdx][1]); // print ones
    }
    
    // btn next part was pressed
    if (nextPartPressed) {
      // erase previous indicator
      lcd.setCursor(printPositions[clockIdx], 1);
      lcd.print("  ");

      // move to next part
      clockIdx = (clockIdx + 1) % 3;

      // break the loop if the user finished editing the seconds
      if (clockIdx == 0)
        break;

      // print the new indicator
      lcd.setCursor(printPositions[clockIdx], 1);
      lcd.print("^^");
    }
  }

  uint8_t hour = parseUInt8(clock[0]);
  uint8_t minute = parseUInt8(clock[1]);
  uint8_t second = parseUInt8(clock[2]);

  while (true) {
    playClock(lcd, hour, minute, second);
  }

  return 0;
}