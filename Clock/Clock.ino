#include <DS3232RTC.h>
#include <LiquidCrystal.h>
#include <Streaming.h>
#include <TimeLib.h>
#include <Wire.h>

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
LiquidCrystal lcd(11, 12, 2, 3, 4, 5);

// constants won't change:
const String days[2][8] = {{"EN","Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"},{"NL", "ma", "di", "wo", "do", "vr", "za", "zo"}};
const String months[2][13] = {{"EN","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"},{"NL","jan","feb","mrt","mei","jun","jul","aug","sep","okt","nov","dec"}};
const String translations[2][4] = {{"EN", "Temperature","Day","Week"},{"NL", "Temperatuur","Dag","Week"}};

/* EXPLANATION DIFFERENT FUNCTIONS FOR CLOCK
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

 * DELIMTERS
':' = delimiter
'-' = delimiter
'/' = delimiter
'.' = delimiter
'|' = delimiter
' ' = delimiter
*/
char rows[4][16] = {{"h:m:s"},{"d D-M-Y"},{"T"},{"w n"}};

typedef struct {
  int hourFormat;           // 12 or 24 hour format
  boolean mondayFirstDay;   // if true, monday will be used as the start of the week; if false, sunday will be used as the start of the week
  String language;          // The language for the weekdays (only needed if you want to display weekday)
  char degreesFormat;       // Celcius or Fahrenheit
  boolean longFormat;       // Display temperature, weeknumber and daynumber with label
  long interval;            // interval at which to refresh lcd (milliseconds)
  long switchPages;         // interval at which to switchPage 1 to 2 (milliseconds)
} Settings;

Settings default_settings = {24,true,"NL",'c',false,1000,30000};
Settings settings = {24,true,"NL",'c',false,1000,30000};

unsigned long previousMillis = 0;        // will store last time lcd was updated
unsigned long oldMillis = 0;             // will store last time lcd switched pages
short DW[2];

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
  if (currentMillis - previousMillis >= settings.interval) {
    // save the last time you refreshed the lcd
    previousMillis = currentMillis;

    // display the date and time according to the specificied order with the specified settings
    displayDateTime(0, 2);
  }
  if (currentMillis - oldMillis >= settings.switchPages) {
    oldMillis = currentMillis;

    displayDateTime(2, 4);
    delay(5000);
  }
}

void displayDateTime(int rowStart, int rowEnd) {
  time_t t;
  t = now();

  // calculate which day and week of the year it is, according to the current time (t)
  DayWeekNumber(year(t),month(t),day(t),weekday(t));

  lcd.clear();
  lcd.setCursor(0,0);
  // for-loop which loops through each row
  for(int row=rowStart;row<rowEnd;row++) {
    // if row == 1, then we are on the second line, so move the cursor of the lcd
    if(not(row%2) == 0) lcd.setCursor(0,1);
    // for-loop which loops through each char in row
    for(int pos=0;pos<15;pos++) {
      displayDesiredFunction(row, pos, t);
    }
  }
  return;
}

void displayDesiredFunction(int row, int pos, time_t t) {
  switch(rows[row][pos]) {
    case 'h':
      displayHours(t);            // display hours (use settings.hourFormat)
      break;
    case 'm':
      printI00(minute(t));        // display minutes
      break;                      
    case 's':
      printI00(second(t));        // display seconds
      break;        
    case 'T':
      displayTemperature();       // display temperature (use settings.temperatureFormat)
      break;
    case 'd':
      displayWeekday(weekday(t)); // display day of week (use settings.mondayFirstDay and lanuague)
      break;        
    case 'D':
      printI00(day(t));           // display day
      break;  
    case 'M':
      printI00(month(t));         // display month
      break;
    case 'S':
      monthShortStr(month(t));    // display month as string
      break;  
    case 'Y':
      printI00(year(t));          // display year
      break;  
    case 'n':
      displayNumber('d');         // display daynumber
      break;  
    case 'w':
      displayNumber('w');        // display weeknumber
      break;  
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

void displayHours(time_t t) {
  (settings.hourFormat==24) ? printI00(hour(t)) : printI00(hourFormat12(t));
  return;
}

void displayTemperature() {
  int tem = RTC.temperature();
  if(settings.longFormat) {
    for(int count=0;count<2;count++) {
      if(translations[count][0] == settings.language) {
        lcd << translations[count][1] << ": ";
        break;
      }
    }
  }
  (settings.degreesFormat == 'c') ? lcd <<int(tem / 4.0) << (char)223 << "C " : lcd << int(tem / 4.0 * 9.0 / 5.0 + 32.0) << (char)223 << "F ";
  return;
}

void displayWeekday(int val)
{
  int correction;
  correction = (settings.mondayFirstDay) ? 1 : 0;
  for(int count=0;count<2;count++) {
    if(days[count][0] == settings.language) {
      lcd << days[count][val-correction];
      break;
    }
  }
  return;
}

void displayNumber(char val) {
  if(val == 'd') {
    if(settings.longFormat == true) {
      for(int count=0;count<2;count++) {
        if(translations[count][0] == settings.language) {
          lcd << translations[count][2] << ": ";
          break;
        }
      }
    }
    printI00(DW[0]);
  }
  else {
    if(settings.longFormat == true) {
      for(int count=0;count<2;count++) {
        if(translations[count][0] == settings.language) {
          lcd << translations[count][3] << ": ";
          break;
        }
      }
    }
    printI00(DW[1]);
  }

}

void monthShortStr(int val) {
  for(int count=0;count<2;count++) {
    if(months[count][0] == settings.language) {
      lcd << months[count][val];
      break;
    }
  }
  return;
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

void DayWeekNumber(unsigned int y, unsigned int m, unsigned int d, unsigned int w){
  int ndays[]={0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};    // Number of days at the beginning of the month in a not leap year.
  //Start to calculate the number of day
  if (m == 1 || m == 2){
    DW[0] = ndays[(m-1)]+d;                     //for any type of year, it calculate the number of days for January or february
  }                        // Now, try to calculate for the other months
  else if ((y % 4 == 0 && y % 100 != 0) ||  y % 400 == 0){  //those are the conditions to have a leap year
   DW[0] = ndays[(m-1)]+d+1;     // if leap year, calculate in the same way but increasing one day
  }
  else {                                //if not a leap year, calculate in the normal way, such as January or February
    DW[0] = ndays[(m-1)]+d;
  }
  // Now start to calculate Week number
  (w==0) ? DW[1] = (DW[0]-7+10)/7 : DW[1] = (DW[0]-w+10)/7;
  return;
}

void setTimeRTC(unsigned int hours, unsigned int minutes, unsigned int seconds, unsigned int d, unsigned int m, unsigned int y) {
  setTime(hours, minutes, seconds, d, m, y);
  RTC.set(now()); 
  return;
}
