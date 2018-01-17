short DW[2];
void DayWeekNumber(unsigned int y, unsigned int m, unsigned int d, unsigned int w) {
  int ndays[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};  // Number of days at the beginning of the month in a not leap year.
  int numLeapDaysToAdd = 0;
  if ((y % 4 == 0 && y % 100 != 0) ||  y % 400 == 0) {
    numLeapDaysToAdd = 1;
  }

  DW[0] = ndays[(m - 1)] + d + numLeapDaysToAdd;

  // Now start to calculate Week number
  if (w == 0)
  {
     DW[1] = (DW[0] - 7 + 10) / 7;
  } else {
     DW[1] = (DW[0] - w + 10) / 7;
  }
}
