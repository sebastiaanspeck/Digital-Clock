/***************************************************************************************
    Name          : Digital Clock
    Author        : Sebastiaan Speck
    Created       : January 14, 2018
    Last Modified : January 19, 2018
    Version       : 2.1
 ***************************************************************************************/

#include <DS3232RTC.h>      // https://github.com/JChristensen/DS3232RTC
#include <Timezone.h>       // https://github.com/JChristensen/Timezone
#include <TimeLib.h>        // https://github.com/PaulStoffregen/Time
#include <Streaming.h>      // http://arduiniana.org/libraries/streaming/
#include <Wire.h>           // https://www.arduino.cc/en/Reference/Wire
#include <LiquidCrystal.h>  // https://www.arduino.cc/en/Reference/LiquidCrystal
#include <DHT.h>            // https://github.com/adafruit/DHT-sensor-library

#define DHTPIN 2     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

DHT dht(DHTPIN, DHTTYPE);

String menuItems[] = {"Homepage", "Tijd", "Datum", "Alarm", "Timer", "Stopwatch", "Wereldklok", "Weer", "Instellingen"};

const char degreesSymbol = 223;

const int kNumLCDCols = 16;
const int kNumLCDRows = 2;
const int kSerialBaud = 9600;
const int kLPin = 13;
const int kNumLCDChars = kNumLCDCols * kNumLCDRows;


/* EXPLANATION DIFFERENT FUNCTIONS FOR CLOCK (WILL ONLY BE USED ON THE HOMEPAGE)
   TIME
   'h' = hours
   'm' = minutes
   's' = seconds
   'a' = alarm 1       (not atm)
   'A' = alarm 2       (not atm)

   WEATHER
   'T' = temperature
   'H' = humidity
   'P' = dew point
   'I' = heat index

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

char default_page[][kNumLCDCols] = {("h:m:s T", "d D-M-Y W")};
char page[4][kNumLCDCols];

const size_t kNumSupportedLanguages = 2;
const size_t kNumDaysPerWeek = 7;
const size_t kNumMonthsPerYear = 12;
const size_t numberOfPages = (sizeof(page) / sizeof(char)) / kNumLCDChars;

const String days[kNumSupportedLanguages][kNumDaysPerWeek] = {{"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}, {"zo", "ma", "di", "wo", "do", "vr", "za"}};
const String months[kNumSupportedLanguages][kNumMonthsPerYear] = {{"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"}, {"jan", "feb", "mrt", "mei", "jun", "jul", "aug", "sep", "okt", "nov", "dec"}};

enum languages_t {EN, NL};
enum degreesFormats_t {CELSIUS, FAHRENHEIT};
enum hourFormats_t {HourFormat12, HourFormat24};
enum firstDayOfWeek_t {SUNDAY, MONDAY};

short DW[2];

typedef struct {
  uint8_t hourFormat;               // 12 or 24 hour format (AM/PM is not displayed)
  uint8_t language;                 // The language for several labels
  uint8_t degreesFormat;            // Celsius or Fahrenheit
  uint8_t firstDayOfWeek;           // Used for the weeknumber
  boolean labels;                   // Display temperature, weeknumber and daynumber with label
} Settings;

Settings default_settings = {HourFormat24, NL, CELSIUS, MONDAY, true};
Settings settings = {HourFormat24, NL, CELSIUS, MONDAY, true};

int language_id;

// Navigation button variables
int readKey;
int savedDistance = 0;

// Menu control variables
int menuPage = 0;
int maxMenuPages = (sizeof(menuItems) / sizeof(String)) - 2;
int cursorPosition = 0;

int brightness = 200;
boolean NightLight = false;
boolean LCDOff = false;
const int lightsOff = 23;
const int lightsOn = 9;

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

TimeChangeRule myDST = {"MDT", Last, Sun, Mar, 2, 2 * 60};  //Daylight time/Summertime = UTC + 2 hours
TimeChangeRule mySTD = {"MST", Last, Sun, Oct, 2, 1 * 60};  //Standard time/Wintertime = UTC + 1 hours
Timezone myTZ(myDST, mySTD);
TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev

time_t oldTime;

float weather[4];

unsigned long previousMillis = 0;
unsigned long LCDPrevious = 0;
const long refreshRate = 1000;
const long LCDNightLight = 10;    //seconds

void setup() {

  // Initializes serial communication
  Serial.begin(kSerialBaud);

  lcd.begin(kNumLCDCols, kNumLCDRows);

  dht.begin();

  // Creates the byte for the 3 custom characters
  lcd.createChar(0, menuCursor);
  lcd.createChar(1, upArrow);
  lcd.createChar(2, downArrow);

  setSyncProvider(RTC.get);
  if (timeStatus() != timeSet) lcd << ("RTC SYNC FAILED");

  pinMode(kLPin, OUTPUT);
  digitalWrite(kLPin, LOW);

  getLanguageId();
  getWeather();
}

void loop() {
  mainMenuDraw();
  drawCursor();
  operateMainMenu();
}

void getWeather() {
  weather[0] = dht.readTemperature();
  weather[1] = dht.readHumidity();
  weather[2] = calculateDewPoint(weather[0], weather[1]);
  weather[3] = dht.computeHeatIndex(weather[0], weather[1], false);
}

float calculateDewPoint(float t, float h) {
  float a = 17.271;
  float b = 237.7;
  float temp = (a * t) / (b + t) + log(h * 0.01);
  float Td = (b * temp) / (a - temp);
  return Td;
}

// This function will generate the 2 menu items that can fit on the screen. They will change as you scroll through your menu. Up and down arrows will indicate your current menu position.
void mainMenuDraw() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print(menuItems[menuPage]);
  lcd.setCursor(1, 1);
  lcd.print(menuItems[menuPage + 1]);
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
    lcd.print(" ");
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
      case 1:  // This case will execute if the "forward" button is pressed
        button = 0;
        switch (cursorPosition) { // The case that is selected here is dependent on which menu page you are on and where the cursor is.
          case 0:                 // Show homepage
            showPage("h:m:s T", "d D-M-Y W");
            break;
          case 1:                 // Show time
            showPage("h:m:s", "");
            break;
          case 2:                 // Show date
            showPage("d D-M-Y", "w n");
            break;
          case 7:                 // Show weather
            showPage("T H", "P I");
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

void showPage(char* line1, char* line2) {
  int activeButton = 0;
  lcd.clear();
  clearPageArray();

  while (activeButton == 0) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= refreshRate) {
      previousMillis = currentMillis;
      int button;
      time_t l = getTime();
      lcd.setCursor(0, 0);
      getWeather();
      DayWeekNumber(year(l), month(l), day(l), weekday(l));
      NightTime(l);
      turnNightLightOff(l);
      writeToPage(0, line1);
      writeToPage(1, line2);
      displayPage(0, 2, l);
      readKey = analogRead(0);
      if (readKey < 790) {
        delay(100);
        readKey = analogRead(0);
      }
      button = evaluateButton(readKey);
      switch (button) {
        case 1:
          button = 0;
          if (NightLight == false && LCDOff == true) {
            changeBacklight();
            oldTime = getTime();
          }
          break;
        case 4:  // This case will execute if the "back" button is pressed
          button = 0;
          activeButton = 1;
          break;
        case 5:
          button = 0;
          changeBacklight();
          break;
      }
    }
  }
}

void displayPage(int rowStart, int rowEnd, time_t local) {
  lcd.setCursor(0, 0);
  // for-loop which loops through each row
  for (int row = rowStart; row < rowEnd; row++) {
    // if row == odd, then we are on the second line, so move the cursor of the lcd
    if (not(row % 2 == 0)) lcd.setCursor(0, 1);
    // for-loop which loops through each char in row
    for (int pos = 0; pos < 15; pos++) {
      displayDesiredFunction(row, pos, local);
    }
  }
}

void displayDesiredFunction(int row, int pos, time_t l) {
  switch (page[row][pos]) {
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
      displayWeather(0, "T");                // display temperature (use settings.temperatureFormat)
      break;
    case 'H':
      displayHumidity("RH");                // display humidity
      break;
    case 'P':
      displayWeather(2, "DP");                // display dew point (use settings.temperatureFormat)
      break;
    case 'I':
      displayWeather(3, "HI");                // display heat index (use settings.temperatureFormat)
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
      displayNumber('d', "D");               // display daynumber
      break;
    case 'w':
      displayNumber('w', "Wk");               // display weeknumber
      break;
    case 'W':
      displayNumber('w', "");
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
  (settings.hourFormat == HourFormat24) ? printI00(hour(l)) : printI00(hourFormat12(l));
}

void displayWeather(int val, String label) {
  if (settings.labels) {
    lcd << label << ": ";
  }
  lcdDisplayWeather(weather[val]);
}

void displayHumidity(String label) {
  if (settings.labels) {
    lcd << label << ": ";
  }
  lcd << int(weather[1]) << "%";
}

void lcdDisplayWeather(float val) {
  (settings.degreesFormat == CELSIUS) ? lcd << int(val) << degreesSymbol << "C" : lcd << celsiusToFahrenheit(val) << (char)223 << "F";
}

int celsiusToFahrenheit(const float celsius) {
  return static_cast<int>(celsius * 1.8 + 32);
}

void displayWeekday(int val) {
  lcd << days[language_id][val - 1];
}

void displayNumber(char val, String label) {
  if (settings.labels == true && label != "") {
    lcd << label << ": ";
  }
  (val == 'd') ? printI00(DW[0]) : printI00(DW[1]);
}

void displayMonthShortStr(int val) {
  Serial << val << endl;
  lcd << months[language_id][val - 1];
}

void displayLocation() {
  //lcd << settings.location;
}

void getLanguageId() {
  switch (settings.language) {
    case EN:
      language_id = 0;
      break;
    case NL:
      language_id = 1;
  }
}

void printI00(int val) {
  if (val < 10) lcd << '0';
  lcd << _DEC(val);
}

void displayDelimiter(char delim) {
  lcd << delim;
}

void DayWeekNumber(unsigned int y, unsigned int m, unsigned int d, unsigned int w) {
  int ndays[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};  // Number of days at the beginning of the month in a not leap year.
  int numLeapDaysToAdd = 0;
  int firstDayOfTheWeek = 0;
  if ((y % 4 == 0 && y % 100 != 0) ||  y % 400 == 0) {
    numLeapDaysToAdd = 1;
  }

  DW[0] = ndays[(m - 1)] + d + numLeapDaysToAdd;

  // Now start to calculate Week number
  (settings.firstDayOfWeek == MONDAY) ? firstDayOfTheWeek = 1 : firstDayOfTheWeek = 0;
  if (w == firstDayOfTheWeek)
  {
     DW[1] = (DW[0] - 7 + 10) / 7;
  } else {
     DW[1] = (DW[0] - w + 10) / 7;
  }
}

void writeToPage(int row, char* val) {
  strcpy(page[row], val);
}

void clearPageArray() {
  memset(page, 0, sizeof(page));
}

time_t getTime() {
  time_t utc, local;
  utc = now();

  local = myTZ.toLocal(utc, &tcr);
  return local;
}

void changeBacklight() {
  brightness = (brightness == 200) ? 0 : 200;
  LCDOff = (LCDOff == true) ? false : true;
  analogWrite(10, brightness);
}

void NightTime(time_t l) {
  if (hour(l) == lightsOff && LCDOff == false) {
    changeBacklight();
  }
  if (hour(l) == lightsOn && LCDOff == true) {
    changeBacklight();
  }
}

void turnNightLightOff(time_t l) {
  if (l - oldTime >= LCDNightLight && oldTime > 0) {
    changeBacklight();
    NightLight == false;
    oldTime = 0;
  }
}
