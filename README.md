# Digital Clock

This is the digital clock you have always dreamed of. This clock is fully customizable and can be used in different languages.

## Features

### Current features
- Display time (duhh)
- Display date (different formats)
- Display temperature (Celcius and Fahrenheit)
- Display day of the week (English and Dutch)
- Display weeknumber
- Display number of day in the year
-	Change summer<->wintertime ([more info](https://en.wikipedia.org/wiki/Summer_Time_in_Europe))

### Roadmap
 OK | Description | Current state | Target
 ----------- | ----------- | ------------- | ------
 :+1: | Change summer<->wintertime ([more info](https://en.wikipedia.org/wiki/Summer_Time_in_Europe)) | Added on 13-01-2018 | Already added
 :-1: | Menu to change settings (see [settings](https://github.com/sebastiaanspeck/Digital-Clock#settings)) | This will be implemented as soon as I have a LCD+Keypad Shield | End of January 2018
 :-1: | Display/set alarm | This will be implemented as soon as I have a LCD+Keypad Shield | End of January 2018
 :-1: | Timer | This will be implemented as soon as I have a LCD+Keypad Shield | End of January 2018
 :-1: | Stopwatch with rounds | This will be implemented as soon as I have a LCD+Keypad Shield | End of January 2018
 :-1: | Local time of different locations (world clock) | This will be implemented as soon as I have a LCD+Keypad Shield | End of January 2018
 :-1: | Display humidity (%) | This will be implemented as soon as I have a temperature and humidity sensor | End of January 2018
 :-1: | Display dew point (Celcius and Fahrenheit) | This will be implemented as soon as I have a temperature and humidity sensor | End of January 2018

## Settings
This digital clock is different because you can control the look and feels of the clock using the five buttons that are on the shield.
### Current settings
Settings name | Description
------------- | -----------
hourFormat    | 12 or 24 hour format (12 hour format doesn't print AM/PM)
language      | The language for several labels
degreesFormat | Use Celcius or Fahrenheit for temperature and dew point
longFormat    | Display several objects with label (see Clock.txt for all labels)
interval      | Interval at which to refresh lcd (milliseconds)
switchPages   | Interval at which to switchPage 1 to 2 (milliseconds) [Only if you have more than 1 page defined]

### Settings  to be implemented
Settings name | Description
------------- | -----------
Summer/Wintertime | Several settings to set the correct time during summer and winter

## Special thanks to
I want to mention several people who helped me developing this project.

### Libraries
During this project, I used several libraries
#### 3rd party libraries  
[DS3232RTC - Made by JChristensen](https://github.com/JChristensen/DS3232RTC) [Used for RTC]   
[Timezone - Made by JChristensen](https://github.com/JChristensen/Timezone) [Used for DST and Worldclock]  
[Streaming - Made by Mikal Hart](http://arduiniana.org/libraries/streaming/) [Used for simple printing on the LCD]  
[TimeLib - Made by PaulStoffregen](https://github.com/PaulStoffregen/Time) [Used for time]

#### Standard libraries
[Wire](https://www.arduino.cc/en/Reference/Wire) [Used for communication with I2C devices (DS3232 RTC)]  
[LiquidCrystal](https://www.arduino.cc/en/Reference/LiquidCrystal) [Used for controlling LCD]
