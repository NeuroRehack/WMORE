void menuTimeStamp()
{
  while (1)
  {
    SerialPrintln(F(""));
    SerialPrintln(F("Menu: Configure Time Stamp"));

    SerialPrint(F("1) Log Date: "));
    if (settings.logDate == true) SerialPrintln(F("Enabled"));
    else SerialPrintln(F("Disabled"));

    SerialPrint(F("2) Log Time: "));
    if (settings.logTime == true) SerialPrintln(F("Enabled"));
    else SerialPrintln(F("Disabled"));

    if (settings.logDate == true || settings.logTime == true)
    {
      SerialPrintln(F("3) Set RTC to compiler macro time"));
    }

    if (settings.logDate == true)
    {
      SerialPrintln(F("4) Manually set RTC date"));

      SerialPrint(F("5) Toggle date style: "));
      if (settings.dateStyle == 0) SerialPrintln(F("mm/dd/yyyy"));
      else if (settings.dateStyle == 1) SerialPrintln(F("dd/mm/yyyy"));
      else if (settings.dateStyle == 2) SerialPrintln(F("yyyy/mm/dd"));
      else SerialPrintln(F("ISO 8601"));
    }

    if (settings.logTime == true)
    {
      SerialPrintln(F("6) Manually set RTC time"));

      SerialPrint(F("7) Toggle time style: "));
      if (settings.hour24Style == true) SerialPrintln(F("24 hour"));
      else SerialPrintln(F("12 hour"));
    }

    if (settings.logDate == true || settings.logTime == true)
    {
      SerialPrint(F("8) Local offset from UTC: "));
      Serial.println(settings.localUTCOffset);
      if (settings.useTxRxPinsForTerminal == true)
        Serial1.println(settings.localUTCOffset);
    }

    SerialPrint(F("9) Log Microseconds: "));
    if (settings.logMicroseconds == true) SerialPrintln(F("Enabled"));
    else SerialPrintln(F("Disabled"));

    SerialPrintln(F("x) Exit"));

    int incoming = getNumber(menuTimeout); //Timeout after x seconds

    if (incoming == 1)
      settings.logDate ^= 1;
    else if (incoming == 2)
      settings.logTime ^= 1;
    else if (incoming == 9)
      settings.logMicroseconds ^= 1;
    else if (incoming == STATUS_PRESSED_X)
      return;
    else if (incoming == STATUS_GETNUMBER_TIMEOUT)
      return;

    if ((settings.logDate == true) || (settings.logTime == true))
    {
      //Options 3, 8
      if (incoming == 3)
      {
        myRTC.setToCompilerTime(); //Set RTC using the system __DATE__ and __TIME__ macros from compiler
        SerialPrintln(F("RTC set to compiler time"));
      }
      else if (incoming == 8)
      {
        SerialPrint(F("Enter the local hour offset from UTC (-12 to 14): "));
        float offset = (float)getDouble(menuTimeout); //Timeout after x seconds
        if (offset < -12 || offset > 14)
          SerialPrintln(F("Error: Offset is out of range"));
        else
          settings.localUTCOffset = offset;
      }
    }

    if (settings.logDate == true)
    {
      if (incoming == 4)
      {
        SerialPrint(F("Enter year (2000-2099): "));
        int year = getNumber(menuTimeout) + 2000;
        if (year < 2000 || year > 2099)
        {
          SerialPrintln(F("Error: Year out of range"));
          continue;
        }
        SerialPrint(F("Enter month (1-12): "));
        int month = getNumber(menuTimeout);
        if (month < 1 || month > 12)
        {
          SerialPrintln(F("Error: Month out of range"));
          continue;
        }
        SerialPrint(F("Enter day (1-31): "));
        int day = getNumber(menuTimeout);
        if (day < 1 || day > 31)
        {
          SerialPrintln(F("Error: Day out of range"));
          continue;
        }
        myRTC.setTime(0, 0, 0, 0, day, month, (year - 2000)); //Manually set RTC
        SerialPrintln(F("RTC date set"));
      }
      else if (incoming == 5)
      {
        settings.dateStyle++;
        if (settings.dateStyle > 3) settings.dateStyle = 0;
      }
    }

    if (settings.logTime == true)
    {
      if (incoming == 6)
      {
        SerialPrint(F("Enter hour (0-23): "));
        int hour = getNumber(menuTimeout);
        if (hour < 0 || hour > 23)
        {
          SerialPrintln(F("Error: Hour out of range"));
          continue;
        }
        SerialPrint(F("Enter minute (0-59): "));
        int minute = getNumber(menuTimeout);
        if (minute < 0 || minute > 59)
        {
          SerialPrintln(F("Error: Minute out of range"));
          continue;
        }
        SerialPrint(F("Enter second (0-59): "));
        int second = getNumber(menuTimeout);
        if (second < 0 || second > 59)
        {
          SerialPrintln(F("Error: Second out of range"));
          continue;
        }
        myRTC.setTime(0, second, minute, hour, myRTC.dayOfMonth, myRTC.month, myRTC.year); //Manually set RTC
        SerialPrintln(F("RTC time set"));
      }
      else if (incoming == 7)
      {
        settings.hour24Style ^= 1;
      }
    }
  }
}
