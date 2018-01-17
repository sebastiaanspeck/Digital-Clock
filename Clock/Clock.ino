#include <DS3232RTC.h>      // https://github.com/JChristensen/DS3232RTC
#include <Timezone.h>       // https://github.com/JChristensen/Timezone
#include <TimeLib.h>        // https://github.com/PaulStoffregen/Time
#include <Streaming.h>      // http://arduiniana.org/libraries/streaming/
#include <Wire.h>           // https://www.arduino.cc/en/Reference/Wire
#include <LiquidCrystal.h>  // https://www.arduino.cc/en/Reference/LiquidCrystal
#include <DHT.h>            // https://github.com/adafruit/DHT-sensor-library
#include "Date.h"

#define DHTPIN 2     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino

// Creates 3 custom characters for the menu display
byte downArrow[8] = {
  B00100, B00100, B00100, B00100, B10101, B01110, B00100, B00100
};

byte upArrow[8] = {
  B00100, B01110, B10101, B00100, B00100, B00100, B00100, B00100
};

byte menuCursor[8] = {
  B01000, B01100, B01110, B01111, B01110, B01100, B01000, B00000
};

// constants won't change:
const String days[] = {"zo", "ma", "di", "wo", "do", "vr", "za"};
const String months[] = {"jan", "feb", "mrt", "mei", "jun", "jul", "aug", "sep", "okt", "nov", "dec"};
const String labels[] = {"T", "RH", "DP", "D", "Wk"};
const String menuItems[] = {"Homepage", "Tijd", "Datum", "Alarm", "Timer", "Stopwatch", "Wereldklok", "Weer", "Instellingen"};

/* EXPLANATION DIFFERENT FUNCTIONS FOR CLOCK (WILL ONLY BE USED ON THE HOMEPAGE)
   TIME
   'h' = hours
   'm' = minutes
   's' = seconds
   'a' = alarm 1       (not atm)
   'A' = alarm 2       (not atm)

   WEATHER
   'T' = temperature
   'H' = humidity      (no sensor atm)
   'P' = dew point     (no sensor atm)

   DATE
   'd' = day of week
   'D' = day
   'M' = month
   'S' = month (short string)
   'Y' = year
   'w' = weeknumber
   'n' = daynumber

   MISCELLANEOUS
   'l' = current location  (need to create a array of enums/strings which can be used in Settings)

   DELIMTERS
   ':' = delimiter
   '-' = delimiter
   '/' = delimiter
   '.' = delimiter
   '|' = delimiter
   ' ' = delimiter
*/
char homepage[][16] = {{"h:m:s w"}, {"d D-M-Y"}};
char timePage[][16] = {{"h:m:s"}, {}};

typedef struct {
  int hourFormat;               // 12 or 24 hour format (AM/PM is not displayed)
  char degreesFormat;           // Celcius or Fahrenheit
  boolean labels;               // Display temperature, weeknumber and daynumber with label
  long refreshRate;             // interval at which to refresh lcd (milliseconds)
  long switchPages;             // interval at which to switchPage 1 to 2 (milliseconds)
} Settings;

Settings default_settings = {24, 'c', true, 1000, 30000};
Settings settings = {24, 'c', true, 1000, 30000};

TimeChangeRule myDST = {"MDT", Last, Sun, Mar, 2, 2 * 60};  //Daylight time/Summertime = UTC + 2 hours
TimeChangeRule mySTD = {"MST", Last, Sun, Oct, 2, 1 * 60};  //Standard time/Wintertime = UTC + 1 hours
Timezone myTZ(myDST, mySTD);
TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev

enum states {
  HOMEPAGE,
  MENU,
  TIME,
  DATE,
  ALARM,
  TIMER,
  STOPWATCH,
  WORLDCLOCK,
  WEATHER,
  SETTINGS
};

unsigned long previousMillis = 0;        // will store last time lcd was updated (page 1)
unsigned long oldMillis = 0;             // will store last time lcd switched pages
int rowX = 0;
int rowY = 2;
int numberOfPages = (sizeof(homepage) / sizeof(char)) / 32;
int lcd_key     = 0;
int adc_key_in  = 0;

// Navigation button variables
int readKey;
int savedDistance = 0;
int buttonDelay = 100;

// Menu control variables
int menuPage = 0;
int maxMenuPages = (sizeof(menuItems) / sizeof(String)) - 2;
int cursorPosition = 0;

states currentState = HOMEPAGE;
float tem;
float hum;
float dew;

bool lcdCleared = false;

void setup() {
  Serial.begin(9600);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  lcd.clear();

  // setSyncProvider() causes the Time library to synchronize with the
  // external RTC by calling RTC.get() every five minutes by default.
  setSyncProvider(RTC.get);
  if (timeStatus() != timeSet) lcd << ("RTC SYNC FAILED");

  lcd.createChar(0, menuCursor);
  lcd.createChar(1, upArrow);
  lcd.createChar(2, downArrow);

  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

}

void loop() {
  // check to see if it's time to refresh the lcd; that is, if the difference
  // between the current time and last time you refreshed the lcd is bigger than
  // the interval at which you want to refresh the lcd.
  mainLoop();
}

void mainLoop() {
  tem = dht.readTemperature();
  hum = dht.readHumidity();

  int activeButton = 0;
  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 4:
        currentState = HOMEPAGE;
        break;
      case 5:
        currentState = MENU;
    }

    switch (currentState) {
      case HOMEPAGE:
        homePage();
        break;
      case MENU:
        rowX = 0;
        rowY = 2;
        menu();
        break;
      case TIME:
        rowX = 0;
        rowY = 2;
        displayTime();
    }
  }
}

void homePage() {
  if (lcdCleared == false) {
    lcd.clear();
    lcdCleared = true;
  }
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= settings.refreshRate) {
    // save the last time you refreshed the lcd
    previousMillis = currentMillis;

    // display the date and time according to the specificied order with the specified settings
    displayHomepage(rowX, rowY);
  }
  if (currentMillis - oldMillis >= settings.switchPages) {
    oldMillis = currentMillis;

    if (rowY == numberOfPages * 2) {
      rowX = 0;
      rowY = 2;
      lcd.clear();
    } else {
      rowX += 2;
      rowY += 2;
      lcd.clear();
    }
  }
}

void displayHomepage(int rowStart, int rowEnd) {

  time_t utc, local;
  utc = now();

  local = myTZ.toLocal(utc, &tcr);

  // calculate which day and week of the year it is, according to the current local time
  DayWeekNumber(year(local), month(local), day(local), weekday(local));

  lcd.setCursor(0, 0);
  // for-loop which loops through each row
  for (int row = rowStart; row < rowEnd; row++) {
    // if row == odd, then we are on the second line, so move the cursor of the lcd
    if (not(row % 2) == 0) lcd.setCursor(0, 1);
    // for-loop which loops through each char in row
    for (int pos = 0; pos < 15; pos++) {
      displayDesiredFunctionHomepage(row, pos, local);
    }
  }
  return;
}

void displayDesiredFunctionHomepage(int row, int pos, time_t l) {
  switch (homepage[row][pos]) {
    case 'h':
      displayHours(l);                  // display hours (use settings.hourFormat)
      break;
    case 'm':
      printI00(minute(l));              // display minutes
      break;
    case 's':
      printI00(second(l));              // display seconds
      break;
    case 'T':
      displayTemperature();             // display temperature (use settings.temperatureFormat)
      break;
    case 'H':
      displayHumidity();                // display humidity
      break;
    case 'P':
      displayDewPoint();                // display dew point (use settings.temperatureFormat)
      break;
    case 'd':
      displayWeekday(weekday(l));       // display day of week (use settings.lanuague)
      break;
    case 'D':
      printI00(day(l));                 // display day
      break;
    case 'M':
      printI00(month(l));               // display month
      break;
    case 'S':
      displayMonthShortStr(month(l));   // display month as string
      break;
    case 'Y':
      printI00(year(l));                // display year
      break;
    case 'n':
      displayNumber('d');               // display daynumber
      break;
    case 'w':
      displayNumber('w');               // display weeknumber
      break;
    case 'l':
      displayLocation();                // display current location
      break;
    case ':':
      displayDelimiter(':');            // display ':'
      break;
    case '-':
      displayDelimiter('-');            // display '-'
      break;
    case '/':
      displayDelimiter('/');            // display '/'
      break;
    case '.':
      displayDelimiter('.');            // display '.'
      break;
    case '|':
      displayDelimiter('|');            // display '|'
      break;
    case ' ':
      displayDelimiter(' ');            // display ' '
      break;
  }
}

void displayHours(time_t l) {
  (settings.hourFormat == 24) ? printI00(hour(l)) : printI00(hourFormat12(l));
  return;
}

void displayTemperature() {
  if (settings.labels) {
    lcd << labels[0] << ": ";
  }
  (settings.degreesFormat == 'c') ? lcd << int(tem) << (char)223 << "C" : lcd << int(((tem * 9) / 5) + 32) << (char)223 << "F";
}

void displayHumidity() {
  if (settings.labels) {
    lcd << labels[1] << ": ";
  }
  lcd << int(hum) << "%";
}

void displayDewPoint() {
  dew = calculateDewPoint(tem, hum);
  if (settings.labels) {
    lcd << labels[2] << ": ";
  }
  (settings.degreesFormat == 'c') ? lcd << int(dew) << (char)223 << "C" : lcd << int(((dew * 9) / 5) + 32) << (char)223 << "F";
}

float calculateDewPoint(float t, float h) {
  float a = 17.271;
  float b = 237.7;
  float temp = (a * t) / (b + t) + log(h * 0.01);
  float Td = (b * temp) / (a - temp);
  return Td;
}

void displayWeekday(int val) {
  lcd << days[val - 1];
  return;
}

void displayNumber(char val) {
  if (val == 'd') {
    if (settings.labels == true) {
      lcd << labels[3] << ": ";
    }
    printI00(DW[0]);
  }
  else {
    if (settings.labels == true) {
      lcd << labels[4] << ": ";
    }
    printI00(DW[1]);
  }
  return;
}

void displayMonthShortStr(int val) {
  lcd << months[val];
  return;
}

void displayLocation() {
  //lcd << settings.location;
}

void printI00(int val) {
  if (val < 10) lcd << '0';
  lcd << _DEC(val);
  return;
}

void displayDelimiter(char delim) {
  lcd << delim;
  return;
}

void setTimeRTC(unsigned int hours, unsigned int minutes, unsigned int seconds, unsigned int d, unsigned int m, unsigned int y) {
  setTime(hours, minutes, seconds, d, m, y);
  RTC.set(now());
  return;
}

void menu() {
  mainMenuDraw();
  drawCursor();
  operateMainMenu();
}

void mainMenuDraw() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd << menuItems[menuPage];
  lcd.setCursor(1, 1);
  lcd << menuItems[menuPage + 1];
  if (menuPage == 0) {
    lcd.setCursor(15, 1);
    lcd.write(byte(2));
  } else if (menuPage > 0 and menuPage < maxMenuPages) {
    lcd.setCursor(15, 1);
    lcd.write(byte(2));
    lcd.setCursor(15, 0);
    lcd.write(byte(1));
  } else if (menuPage == maxMenuPages) {
    lcd.setCursor(15, 0);
    lcd.write(byte(1));
  }
}

// When called, this function will erase the current cursor and redraw it based on the cursorPosition and menuPage variables.
void drawCursor() {
  for (int x = 0; x < 2; x++) {     // Erases current cursor
    lcd.setCursor(0, x);
    lcd << " ";
  }

  // The menu is set up to be progressive (menuPage 0 = Item 1 & Item 2, menuPage 1 = Item 2 & Item 3, menuPage 2 = Item 3 & Item 4), so
  // in order to determine where the cursor should be you need to see if you are at an odd or even menu page and an odd or even cursor position.
  if (menuPage % 2 == 0) {
    if (cursorPosition % 2 == 0) {  // If the menu page is even and the cursor position is even that means the cursor should be on line 1
      lcd.setCursor(0, 0);
      lcd.write(byte(0));
    }
    if (cursorPosition % 2 != 0) {  // If the menu page is even and the cursor position is odd that means the cursor should be on line 2
      lcd.setCursor(0, 1);
      lcd.write(byte(0));
    }
  }
  if (menuPage % 2 != 0) {
    if (cursorPosition % 2 == 0) {  // If the menu page is odd and the cursor position is even that means the cursor should be on line 2
      lcd.setCursor(0, 1);
      lcd.write(byte(0));
    }
    if (cursorPosition % 2 != 0) {  // If the menu page is odd and the cursor position is odd that means the cursor should be on line 1
      lcd.setCursor(0, 0);
      lcd.write(byte(0));
    }
  }
}

void operateMainMenu() {
  int activeButton = 0;
  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 0: // When button returns as 0 there is no action taken
        break;
      case 1:  // This case will execute if the "forward"/right button is pressed
        button = 0;
        switch (cursorPosition) { // The case that is selected here is dependent on which menu page you are on and where the cursor is.
          case 0:
            currentState = HOMEPAGE;
            lcdCleared = false;
            break;
          case 1:
            currentState = TIME;
            lcdCleared = false;
            break;
          case 2:
            menuDate();
            break;
          case 3:
            menuAlarm();
            break;
          case 4:
            menuTimer();
            break;
          case 5:
            menuStopwatch();
            break;
          case 6:
            menuWorldclock();
            break;
          case 7:
            menuWeather();
            break;
          case 8:
            menuSettings();
            break;
        }
        activeButton = 1;
        mainMenuDraw();
        drawCursor();
        break;
      case 2:
        button = 0;
        if (menuPage == 0) {
          cursorPosition = cursorPosition - 1;
          cursorPosition = constrain(cursorPosition, 0, ((sizeof(menuItems) / sizeof(String)) - 1));
        }
        if (menuPage % 2 == 0 and cursorPosition % 2 == 0) {
          menuPage = menuPage - 1;
          menuPage = constrain(menuPage, 0, maxMenuPages);
        }

        if (menuPage % 2 != 0 and cursorPosition % 2 != 0) {
          menuPage = menuPage - 1;
          menuPage = constrain(menuPage, 0, maxMenuPages);
        }

        cursorPosition = cursorPosition - 1;
        cursorPosition = constrain(cursorPosition, 0, ((sizeof(menuItems) / sizeof(String)) - 1));

        mainMenuDraw();
        drawCursor();
        activeButton = 1;
        break;
      case 3:
        button = 0;
        if (menuPage % 2 == 0 and cursorPosition % 2 != 0) {
          menuPage = menuPage + 1;
          menuPage = constrain(menuPage, 0, maxMenuPages);
        }

        if (menuPage % 2 != 0 and cursorPosition % 2 == 0) {
          menuPage = menuPage + 1;
          menuPage = constrain(menuPage, 0, maxMenuPages);
        }

        cursorPosition = cursorPosition + 1;
        cursorPosition = constrain(cursorPosition, 0, ((sizeof(menuItems) / sizeof(String)) - 1));
        mainMenuDraw();
        drawCursor();
        activeButton = 1;
        break;
      case 4:
        currentState = HOMEPAGE;
        lcdCleared = false;
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

// This function is called whenever a button press is evaluated. The LCD shield works by observing a voltage drop across the buttons all hooked up to A0.
int evaluateButton(int x) {
  if (x < 50)  return 1; // right
  if (x < 250) return 2; // up
  if (x < 450) return 3; // down
  if (x < 650) return 4; // left
  if (x < 850) return 5; // select
  return 0;
}

void displayTime() { // Function executes when you select the 2nd item from main menu
  int activeButton = 0;

  if (lcdCleared == false) {
    lcd.clear();
    lcdCleared = true;
  }
  while (activeButton == 0) {
    int button;
    unsigned long currentMillis = millis();
    readKey = analogRead(0);
    if (readKey < 850) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
    if (currentMillis - previousMillis >= settings.refreshRate) {
      // save the last time you refreshed the lcd
      previousMillis = currentMillis;

      // display the date and time according to the specificied order with the specified settings
      displayTimePage(rowX, rowY);
    }
  }
}

void displayTimePage(int rowStart, int rowEnd) {
  time_t utc, local;
  utc = now();

  local = myTZ.toLocal(utc, &tcr);

  // calculate which day and week of the year it is, according to the current local time
  DayWeekNumber(year(local), month(local), day(local), weekday(local));

  lcd.setCursor(0, 0);
  // for-loop which loops through each row
  for (int row = rowStart; row < rowEnd; row++) {
    // if row == odd, then we are on the second line, so move the cursor of the lcd
    if (not(row % 2) == 0) lcd.setCursor(0, 1);
    // for-loop which loops through each char in row
    for (int pos = 0; pos < 15; pos++) {
      displayDesiredFunctionTimePage(row, pos, local);
    }
  }
  return;
}

void displayDesiredFunctionTimePage(int row, int pos, time_t l) {
  switch (timePage[row][pos]) {
    case 'h':
      displayHours(l);                  // display hours (use settings.hourFormat)
      break;
    case 'm':
      printI00(minute(l));              // display minutes
      break;
    case 's':
      printI00(second(l));              // display seconds
      break;
    case 'T':
      displayTemperature();             // display temperature (use settings.temperatureFormat)
      break;
    case 'H':
      displayHumidity();                // display humidity
      break;
    case 'P':
      displayDewPoint();                // display dew point (use settings.temperatureFormat)
      break;
    case 'd':
      displayWeekday(weekday(l));       // display day of week (use settings.lanuague)
      break;
    case 'D':
      printI00(day(l));                 // display day
      break;
    case 'M':
      printI00(month(l));               // display month
      break;
    case 'S':
      displayMonthShortStr(month(l));   // display month as string
      break;
    case 'Y':
      printI00(year(l));                // display year
      break;
    case 'n':
      displayNumber('d');               // display daynumber
      break;
    case 'w':
      displayNumber('w');               // display weeknumber
      break;
    case 'l':
      displayLocation();                // display current location
      break;
    case ':':
      displayDelimiter(':');            // display ':'
      break;
    case '-':
      displayDelimiter('-');            // display '-'
      break;
    case '/':
      displayDelimiter('/');            // display '/'
      break;
    case '.':
      displayDelimiter('.');            // display '.'
      break;
    case '|':
      displayDelimiter('|');            // display '|'
      break;
    case ' ':
      displayDelimiter(' ');            // display ' '
      break;
  }
}

void menuDate() { // Function executes when you select the 3rd item from main menu
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(3, 0);
  lcd << "Sub Menu 3";

  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 850) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

void menuAlarm() { // Function executes when you select the 4th item from main menu
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(3, 0);
  lcd << "Sub Menu 4";

  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 850) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

void menuTimer() { // Function executes when you select the 5th item from main menu
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(3, 0);
  lcd << "Sub Menu 5";

  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 850) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

void menuStopwatch() { // Function executes when you select the 6th item from main menu
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(3, 0);
  lcd << "Sub Menu 6";

  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 850) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

void menuWorldclock() { // Function executes when you select the 7th item from main menu
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(3, 0);
  lcd << "Sub Menu 7";

  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 850) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

void menuWeather() { // Function executes when you select the 8th item from main menu
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(3, 0);
  lcd << "Sub Menu 8";

  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 850) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

void menuSettings() { // Function executes when you select the 9th item from main menu
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(3, 0);
  lcd << "Sub Menu 9";

  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 850) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

