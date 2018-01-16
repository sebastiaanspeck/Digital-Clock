short DW[2];

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
