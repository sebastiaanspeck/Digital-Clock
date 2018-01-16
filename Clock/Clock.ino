#include <DS3232RTC.h>      // https://github.com/JChristensen/DS3232RTC
#include <Timezone.h>       // https://github.com/JChristensen/Timezone
#include <TimeLib.h>        // https://github.com/PaulStoffregen/Time
#include <Streaming.h>      // http://arduiniana.org/libraries/streaming/
#include <Wire.h>           // https://www.arduino.cc/en/Reference/Wire
#include <LiquidCrystal.h>  // https://www.arduino.cc/en/Reference/LiquidCrystal
#include "Date.h"

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
LiquidCrystal lcd(11, 12, 2, 3, 4, 5);

// constants won't change:
const String days[2][7] = {{"Sun","Mon", "Tue", "Wed", "Thu", "Fri", "Sat"},{"zo","ma", "di", "wo", "do", "vr", "za"}};
const String months[2][12] = {{"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"},{"jan","feb","mrt","mei","jun","jul","aug","sep","okt","nov","dec"}};
const String translations[2][3] = {{"Temp","D","Wk"},{"Temp","D","Wk"}};

/* EXPLANATION DIFFERENT FUNCTIONS FOR CLOCK (WILL ONLY BE USED ON THE HOMEPAGE)
 * TIME
'h' = hours
'm' = minutes
's' = seconds
'a' = alarm 1       (not atm)
'A' = alarm 2       (not atm)

 * WEATHER
'T' = temperature
'H' = humidity      (no sensor atm)
'P' = dew point     (no sensor atm)

 * DATE
'd' = day of week
'D' = day
'M' = month
'S' = month (short string)
'Y' = year
'w' = weeknumber
'n' = daynumber

 * MISCELLANEOUS
'l' = current location  (need to create a array of enums/strings which can be used in Settings)

 * DELIMTERS
':' = delimiter
'-' = delimiter
'/' = delimiter
'.' = delimiter
'|' = delimiter
' ' = delimiter
*/
char homepage[4][16] = {{"h:m:s"},{"d D-M-Y"},{"T"},{"w n"}};

enum languages_t {EN, NL}; 

typedef struct {
  int hourFormat;               // 12 or 24 hour format (AM/PM is not displayed)
  uint8_t language;             // The language for several labels
  char degreesFormat;           // Celcius or Fahrenheit
  boolean labels;               // Display temperature, weeknumber and daynumber with label
  long intervalPage1;           // interval at which to refresh lcd (milliseconds)
  long switchPages;             // interval at which to switchPage 1 to 2 (milliseconds)
  long intervalPage2;           // interval at which to switchPage 2 to 1 (milliseconds)
} Settings;

Settings default_settings = {24,NL,'c',true,1000,30000,5000};
Settings settings = {24,NL,'c',true,1000,30000,5000};

//typedef struct {
//    char abbrev[6];     // five chars max
//    uint8_t wk;         // First, Second, Third, Fourth, or Last week of the month
//    uint8_t dow;        // day of week, 1=Sun, 2=Mon, ... 7=Sat
//    uint8_t mon;        // 1=Jan, 2=Feb, ... 12=Dec
//    uint8_t hour;       // 0-23
//    int offset;         // offset from UTC in hours
//} TimeSettings;
//
//TimeSettings settings_DST = {"MDT", Last, Sun, Mar, 2, 2};
//TimeSettings settings_STD = {"MST", Last, Sun, Oct, 2, 1};

// need to use the settings from TimeSettings and make it changeable (in function)
//TimeChangeRule myDST = {settings_DST.abbrev, settings_DST.wk, settings_DST.dow, settings_DST.mon, settings_DST.hour, settings_DST.offset*60};    //Daylight time/Summertime = UTC + 2 hours
//TimeChangeRule mySTD = {settings_STD.abbrev, settings_STD.wk, settings_STD.dow, settings_STD.mon, settings_STD.hour, settings_STD.offset*60};    //Standard time/Wintertime = UTC + 1 hours

TimeChangeRule myDST = {"MDT", Last, Sun, Mar, 2, 2*60};    //Daylight time/Summertime = UTC + 2 hours
TimeChangeRule mySTD = {"MST", Last, Sun, Oct, 2, 1*60};    //Standard time/Wintertime = UTC + 1 hours
Timezone myTZ(myDST, mySTD);
TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev

unsigned long previousMillis1 = 0;       // will store last time lcd was updated (page 1)
unsigned long oldMillis = 0;             // will store last time lcd switched pages
int language_id;

void setup() {
  Serial.begin(9600);
  
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  
  // setSyncProvider() causes the Time library to synchronize with the
  // external RTC by calling RTC.get() every five minutes by default.
  setSyncProvider(RTC.get);        
  if (timeStatus() != timeSet) lcd << ("RTC SYNC FAILED");

  pinMode(13,OUTPUT);
  digitalWrite(13,LOW);
  
}

void loop() {
  // check to see if it's time to refresh the lcd; that is, if the difference
  // between the current time and last time you refreshed the lcd is bigger than
  // the interval at which you want to refresh the lcd.
  unsigned long currentMillis = millis();
  defineLanguageId();
  if (currentMillis - previousMillis1 >= settings.intervalPage1) {
    // save the last time you refreshed the lcd
    previousMillis1 = currentMillis;

    // display the date and time according to the specificied order with the specified settings
    displayPage(0, 2);
  }
  if (currentMillis - oldMillis >= settings.switchPages) {
    oldMillis = currentMillis;

    displayPage(2, 4);

    delay(settings.intervalPage2);
    
  }
}

void displayPage(int rowStart, int rowEnd) {
  time_t utc, local;
  utc = now();

  local = myTZ.toLocal(utc, &tcr);

  // calculate which day and week of the year it is, according to the current local time
  DayWeekNumber(year(local),month(local),day(local),weekday(local));

  lcd.clear();
  lcd.setCursor(0,0);
  // for-loop which loops through each row
  for(int row=rowStart;row<rowEnd;row++) {
    // if row == 1, then we are on the second line, so move the cursor of the lcd
    if(not(row%2) == 0) lcd.setCursor(0,1);
    // for-loop which loops through each char in row
    for(int pos=0;pos<15;pos++) {
      displayDesiredFunction(row, pos, local);
    }
  }
  return;
}

void displayDesiredFunction(int row, int pos, time_t l) {
  switch(homepage[row][pos]) {
    case 'h':
      displayHours(l);            // display hours (use settings.hourFormat)
      break;
    case 'm':
      printI00(minute(l));        // display minutes
      break;                      
    case 's':
      printI00(second(l));        // display seconds
      break;        
    case 'T':
      displayTemperature();       // display temperature (use settings.temperatureFormat)
      break;
    case 'd':
      displayWeekday(weekday(l)); // display day of week (use settings.mondayFirstDay and lanuague)
      break;        
    case 'D':
      printI00(day(l));           // display day
      break;  
    case 'M':
      printI00(month(l));         // display month
      break;
    case 'S':
      displayMonthShortStr(month(l));    // display month as string
      break;  
    case 'Y':
      printI00(year(l));          // display year
      break;  
    case 'n':
      displayNumber('d');         // display daynumber
      break;  
    case 'w':
      displayNumber('w');         // display weeknumber
      break;
    case 'l':
      displayLocation();            // display current location  
    case ':':
      displayDelimiter(':');      // display ':'
      break;  
    case '-':
      displayDelimiter('-');      // display '-'
      break;  
    case '/':
      displayDelimiter('/');      // display '/'
      break;  
    case '.':
      displayDelimiter('.');      // display '.'
      break;
    case '|':
      displayDelimiter('|');      // display '|'
      break;    
    case ' ':
      displayDelimiter(' ');      // display ' '
      break;  
  }
}

void displayHours(time_t l) {
  (settings.hourFormat==24) ? printI00(hour(l)) : printI00(hourFormat12(l));
  return;
}

void displayTemperature() {
  int tem = RTC.temperature();
  if(settings.labels) {
    lcd << translations[language_id][0] << ": ";
  }
  (settings.degreesFormat == 'c') ? lcd <<int(tem / 4.0) << (char)223 << "C " : lcd << int(tem / 4.0 * 9.0 / 5.0 + 32.0) << (char)223 << "F ";
  return;
}

void displayWeekday(int val)
{
  lcd << days[language_id][val-1];
  return;
}

void displayNumber(char val) {
  if(val == 'd') {
    if(settings.labels == true) {
      lcd << translations[language_id][1] << ": ";
    }
    printI00(DW[0]);
  }
  else {
    if(settings.labels == true) {
      lcd << translations[language_id][2] << ": ";
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
  switch(settings.language) {
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

