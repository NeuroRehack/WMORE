//Query the RTC and put the appropriately formatted (according to settings) 
//string into the passed buffer. timeStringBuffer should be at least 37 chars long
//Code modified by @DennisMelamed in PR #70
void getTimeString(char timeStringBuffer[])
{
  //reset the buffer
  timeStringBuffer[0] = '\0';

  myRTC.getTime();

  if (settings.logDate)
  {
    char rtcDate[12]; // 10/12/2019,
    char rtcDay[3];
    char rtcMonth[3];
    char rtcYear[5];
    if (myRTC.dayOfMonth < 10)
      sprintf(rtcDay, "0%d", myRTC.dayOfMonth);
    else
      sprintf(rtcDay, "%d", myRTC.dayOfMonth);
    if (myRTC.month < 10)
      sprintf(rtcMonth, "0%d", myRTC.month);
    else
      sprintf(rtcMonth, "%d", myRTC.month);
    if (myRTC.year < 10)
      sprintf(rtcYear, "200%d", myRTC.year);
    else
      sprintf(rtcYear, "20%d", myRTC.year);
    if (settings.dateStyle == 0)
      sprintf(rtcDate, "%s/%s/%s,", rtcMonth, rtcDay, rtcYear);
    else if (settings.dateStyle == 1)
      sprintf(rtcDate, "%s/%s/%s,", rtcDay, rtcMonth, rtcYear);
    else if (settings.dateStyle == 2)
      sprintf(rtcDate, "%s/%s/%s,", rtcYear, rtcMonth, rtcDay);
    else // if (settings.dateStyle == 3)
      sprintf(rtcDate, "%s-%s-%sT", rtcYear, rtcMonth, rtcDay);
    strcat(timeStringBuffer, rtcDate);
  }

  if ((settings.logTime) || ((settings.logDate) && (settings.dateStyle == 3)))
  {
    char rtcTime[16]; //09:14:37.41, or 09:14:37+00:00,
    int adjustedHour = myRTC.hour;
    if (settings.hour24Style == false)
    {
      if (adjustedHour > 12) adjustedHour -= 12;
    }
    char rtcHour[3];
    char rtcMin[3];
    char rtcSec[3];
    char rtcHundredths[3];
    char timeZoneH[4];
    char timeZoneM[4];
    if (adjustedHour < 10)
      sprintf(rtcHour, "0%d", adjustedHour);
    else
      sprintf(rtcHour, "%d", adjustedHour);
    if (myRTC.minute < 10)
      sprintf(rtcMin, "0%d", myRTC.minute);
    else
      sprintf(rtcMin, "%d", myRTC.minute);
    if (myRTC.seconds < 10)
      sprintf(rtcSec, "0%d", myRTC.seconds);
    else
      sprintf(rtcSec, "%d", myRTC.seconds);
    if (myRTC.hundredths < 10)
      sprintf(rtcHundredths, "0%d", myRTC.hundredths);
    else
      sprintf(rtcHundredths, "%d", myRTC.hundredths);
    if (settings.localUTCOffset >= 0)
    {
      if (settings.localUTCOffset < 10)
        sprintf(timeZoneH, "+0%d", (int)settings.localUTCOffset);
      else
        sprintf(timeZoneH, "+%d", (int)settings.localUTCOffset);
    }
    else
    {
      if (settings.localUTCOffset <= -10)
        sprintf(timeZoneH, "-%d", 0 - (int)settings.localUTCOffset);
      else
        sprintf(timeZoneH, "-0%d", 0 - (int)settings.localUTCOffset);
    }
    int tzMins = (int)((settings.localUTCOffset - (float)((int)settings.localUTCOffset)) * 60.0);
    if (tzMins < 0)
      tzMins = 0 - tzMins;
    if (tzMins < 10)
      sprintf(timeZoneM, ":0%d", tzMins);
    else
      sprintf(timeZoneM, ":%d", tzMins);
    if ((settings.logDate) && (settings.dateStyle == 3))
    {
      sprintf(rtcTime, "%s:%s:%s%s%s,", rtcHour, rtcMin, rtcSec, timeZoneH, timeZoneM);
      strcat(timeStringBuffer, rtcTime);      
    }
    if (settings.logTime)
    {
      sprintf(rtcTime, "%s:%s:%s.%s,", rtcHour, rtcMin, rtcSec, rtcHundredths);
      strcat(timeStringBuffer, rtcTime);
    }
  }
  
  if (settings.logMicroseconds)
  {
    // Convert uint64_t to string
    // Based on printLLNumber by robtillaart
    // https://forum.arduino.cc/index.php?topic=143584.msg1519824#msg1519824
    char microsecondsRev[20]; // Char array to hold to microseconds (reversed order)
    char microseconds[20]; // Char array to hold to microseconds (correct order)
    uint64_t microsNow = micros();
    unsigned int i = 0;
    
    if (microsNow == 0ULL) // if usBetweenReadings is zero, set tempTime to "0"
    {
      microseconds[0] = '0';
      microseconds[1] = ',';
      microseconds[2] = 0;
    }
    
    else
    {
      while (microsNow > 0)
      {
        microsecondsRev[i++] = (microsNow % 10) + '0'; // divide by 10, convert the remainder to char
        microsNow /= 10; // divide by 10
      }
      unsigned int j = 0;
      while (i > 0)
      {
        microseconds[j++] = microsecondsRev[--i]; // reverse the order
        microseconds[j] = ',';
        microseconds[j+1] = 0; // mark the end with a NULL
      }
    }
    
    strcat(timeStringBuffer, microseconds);
  }
}
