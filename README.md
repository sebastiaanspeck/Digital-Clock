# Digital Clock

This is the digital clock you have always dreamed of. This clock is fully customizable and can be used in different languages.

# Feature-list
## Current features
- Display time (duhh)
- Display date (different formats)
- Display temperature (Celcius and Fahrenheit)
- Display day of the week (English and Dutch)
- Display weeknumber
- Display number of day in the year
-	Change summer<->wintertime ([more info](https://en.wikipedia.org/wiki/Summer_Time_in_Europe))

## Roadmap
- [ ] Menu to change settings (see [settings](https://github.com/sebastiaanspeck/Digital-Clock#settings))  
  This will be implemented as soon as I have a LCD+Keypad Shield
- [ ] Display humidity (%)
  This will be implemented as soon as I have a temperature and humidity sensor
- [ ] Display dew point (Celcius en Fahrenheit)
- [ ] Display/set alarm (already possible with predefining it at the start)
- [ ] Timer
- [ ] Stopwatch
- [ ] Local time of different locations (world clock)

# Settings
This digital clock is different because you can control the look and feels of the clock using the five buttons that are on the shield.
## Current SETTINGS

hourFormat    : 12 or 24 hour format (12 hour format doesn't print AM/PM)
language    : The language for several labels
char degreesFormat : Celcius or Fahrenheit
boolean longFormat : Display temperature, weeknumber and daynumber with label
long interval      : Interval at which to refresh lcd (milliseconds)
long switchPages   : Interval at which to switchPage 1 to 2 (milliseconds)

- hourFormat: 12 or 24 hour format (12 hour format doesn't print AM/PM)
- language: The language for several labels (e.a. weekday)  
Current supported languages: English and Dutch
- degreesFormat
