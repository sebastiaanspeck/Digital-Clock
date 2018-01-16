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

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino

// constants won't change:
const String days[2][7] = {{"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}, {"zo", "ma", "di", "wo", "do", "vr", "za"}};
const String months[2][12] = {{"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"}, {"jan", "feb", "mrt", "mei", "jun", "jul", "aug", "sep", "okt", "nov", "dec"}};
const String labels[5] = {"T", "RH", "DP", "D", "Wk"};

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
char homepage[][16] = {{"h:m:s"}, {"d D-M-Y"}, {"w n"}, {"S"}, {"T H"}, {"P"}};

enum languages_t {EN, NL};

typedef struct {
  int hourFormat;               // 12 or 24 hour format (AM/PM is not displayed)
  uint8_t language;             // The language for several labels
  char degreesFormat;           // Celcius or Fahrenheit
  boolean labels;               // Display temperature, weeknumber and daynumber with label
  long intervalPage1;           // interval at which to refresh lcd (milliseconds)
  long switchPages;             // interval at which to switchPage 1 to 2 (milliseconds)
} Settings;

Settings default_settings = {24, NL, 'c', true, 1000, 30000};
Settings settings = {24, NL, 'c', true, 1000, 30000};

TimeChangeRule myDST = {"MDT", Last, Sun, Mar, 2, 2 * 60};  //Daylight time/Summertime = UTC + 2 hours
TimeChangeRule mySTD = {"MST", Last, Sun, Oct, 2, 1 * 60};  //Standard time/Wintertime = UTC + 1 hours
Timezone myTZ(myDST, mySTD);
TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev

unsigned long previousMillis = 0;        // will store last time lcd was updated (page 1)
unsigned long oldMillis = 0;             // will store last time lcd switched pages
int language_id;
int rowX = 0;
int rowY = 2;
int numberOfPages = (sizeof(homepage) / sizeof(char)) / 32;
float tem;
float hum;
float dew;

void setup() {
  Serial.begin(9600);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  // setSyncProvider() causes the Time library to synchronize with the
  // external RTC by calling RTC.get() every five minutes by default.
  setSyncProvider(RTC.get);
  if (timeStatus() != timeSet) lcd << ("RTC SYNC FAILED");


  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

}

void loop() {
  // check to see if it's time to refresh the lcd; that is, if the difference
  // between the current time and last time you refreshed the lcd is bigger than
  // the interval at which you want to refresh the lcd.
  unsigned long currentMillis = millis();
  defineLanguageId();
  tem = dht.readTemperature();
  hum = dht.readHumidity();
  if (currentMillis - previousMillis >= settings.intervalPage1) {
    // save the last time you refreshed the lcd
    previousMillis = currentMillis;

    // display the date and time according to the specificied order with the specified settings
    displayPage(rowX, rowY);
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

void displayPage(int rowStart, int rowEnd) {

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
      displayDesiredFunction(row, pos, local);
    }
  }
  return;
}

void displayDesiredFunction(int row, int pos, time_t l) {
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

void displayWeekday(int val)
{
  lcd << days[language_id][val - 1];
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
  lcd << months[language_id][val];
  return;
}

void displayLocation() {
  //lcd << settings.location;
}

void defineLanguageId() {
  switch (settings.language) {
    case EN:
      language_id = 0;
      break;
    case NL:
      language_id = 1;
  }
}

void printI00(int val)
{
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

