// gfvalvo's flash string helper code: https://forum.arduino.cc/index.php?topic=533118.msg3634809#msg3634809
void SerialPrint(const char *);
void SerialPrint(const __FlashStringHelper *);
void SerialPrintln(const char *);
void SerialPrintln(const __FlashStringHelper *);
void DoSerialPrint(char (*)(const char *), const char *, bool newLine = false);

// #define DUMP( varname ) {Serial.printf("%s: %d\r\n", #varname, varname); if (settings.useTxRxPinsForTerminal == true) Serial1.printf("%s: %d\r\n", #varname, varname);}
// #define SerialPrintf1( var ) {Serial.printf( var ); if (settings.useTxRxPinsForTerminal == true) Serial1.printf( var );}
// #define SerialPrintf2( var1, var2 ) {Serial.printf( var1, var2 ); if (settings.useTxRxPinsForTerminal == true) Serial1.printf( var1, var2 );}
// #define SerialPrintf3( var1, var2, var3 ) {Serial.printf( var1, var2, var3 ); if (settings.useTxRxPinsForTerminal == true) Serial1.printf( var1, var2, var3 );}
// #define SerialPrintf4( var1, var2, var3, var4 ) {Serial.printf( var1, var2, var3, var4 ); if (settings.useTxRxPinsForTerminal == true) Serial1.printf( var1, var2, var3, var4 );}
// #define SerialPrintf5( var1, var2, var3, var4, var5 ) {Serial.printf( var1, var2, var3, var4, var5 ); if (settings.useTxRxPinsForTerminal == true) Serial1.printf( var1, var2, var3, var4, var5 );}


void beginQwiic()
{
  pinMode(PIN_QWIIC_POWER, OUTPUT);
  pin_config(PinName(PIN_QWIIC_POWER), g_AM_HAL_GPIO_OUTPUT); // Make sure the pin does actually get re-configured
  qwiicPowerOn();
  qwiic.begin();
  setQwiicPullups(settings.qwiicBusPullUps); //Just to make it really clear what pull-ups are being used, set pullups here.
}

void setQwiicPullups(uint32_t qwiicBusPullUps)
{
  //Change SCL and SDA pull-ups manually using pin_config
  am_hal_gpio_pincfg_t sclPinCfg = g_AM_BSP_GPIO_IOM1_SCL;
  am_hal_gpio_pincfg_t sdaPinCfg = g_AM_BSP_GPIO_IOM1_SDA;

  if (qwiicBusPullUps == 0)
  {
    sclPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE; // No pull-ups
    sdaPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE;
  }
  else if (qwiicBusPullUps == 1)
  {
    sclPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_1_5K; // Use 1K5 pull-ups
    sdaPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_1_5K;
  }
  else if (qwiicBusPullUps == 6)
  {
    sclPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_6K; // Use 6K pull-ups
    sdaPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_6K;
  }
  else if (qwiicBusPullUps == 12)
  {
    sclPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_12K; // Use 12K pull-ups
    sdaPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_12K;
  }
  else
  {
    sclPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_24K; // Use 24K pull-ups
    sdaPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_24K;
  }

  pin_config(PinName(PIN_QWIIC_SCL), sclPinCfg);
  pin_config(PinName(PIN_QWIIC_SDA), sdaPinCfg);
}

void beginSerialLogging()
{
  if (online.microSD == true && settings.logSerial == true) // TODO settings
  {
    //If we don't have a file yet, create one. Otherwise, re-open the last used file
    if (strlen(serialDataFileName) == 0)
      strcpy(serialDataFileName, findNextAvailableLog(settings.nextSerialLogNumber, "serialLog"));

    if (serialDataFile.open(serialDataFileName, O_CREAT | O_APPEND | O_WRITE) == false)
    {
      SerialPrintln(F("Failed to create serial log file"));
      //systemError(ERROR_FILE_OPEN);
      online.serialLogging = false;
      return;
    }

    updateDataFileCreate(&serialDataFile); // Update the file create time & date
    serialDataFile.sync();

    //We need to manually restore the Serial1 TX and RX pins
    configureSerial1TxRx();

    Serial1.begin(settings.serialLogBaudRate);

    online.serialLogging = true;
  }
  else
    online.serialLogging = false;
}

//Step through the node list and print helper text for the enabled readings
void printHelperText(bool terminalOnly)
{
}

//If certain devices are attached, we need to reduce the I2C max speed
void setMaxI2CSpeed()
{
}

void beginSerialOutput()
{
  if (settings.outputSerial == true)
  {
    //We need to manually restore the Serial1 TX and RX pins
    configureSerial1TxRx();

    Serial1.begin(settings.serialLogBaudRate); // (Re)start the serial port
    online.serialOutput = true;
  }
  else
    online.serialOutput = false;
}

//uint32_t howLongToSleepFor(void)
//{
//  //Counter/Timer 6 will use the 32kHz clock
//  //Calculate how many 32768Hz system ticks we need to sleep for:
//  //sysTicksToSleep = msToSleep * 32768L / 1000
//  //We need to be careful with the multiply as we will overflow uint32_t if msToSleep is > 131072
//
//  //goToSleep will automatically compensate for how long we have been awake
//
//  uint32_t msToSleep;
//
//  if (checkSleepOnFastSlowPin())
//    msToSleep = (uint32_t)(settings.slowLoggingIntervalSeconds * 1000UL);
//  else if (checkSleepOnRTCTime())
//  {
//    // checkSleepOnRTCTime has returned true, so we know that we are between slowLoggingStartMOD and slowLoggingStopMOD
//    // We need to check how long it is until slowLoggingStopMOD (accounting for midnight!) and adjust the sleep duration
//    // if slowLoggingStopMOD occurs before slowLoggingIntervalSeconds
//
//    msToSleep = (uint32_t)(settings.slowLoggingIntervalSeconds * 1000UL); // Default to this
//
//    myRTC.getTime(); // Get the RTC time
//    long secondsOfDay = (myRTC.hour * 60 * 60) + (myRTC.minute * 60) + myRTC.seconds;
//
//    long slowLoggingStopSOD = settings.slowLoggingStopMOD * 60; // Convert slowLoggingStop to seconds-of-day
//
//    long secondsUntilStop = slowLoggingStopSOD - secondsOfDay; // Calculate how long it is until slowLoggingStop
//
//    // If secondsUntilStop is negative then we know that now is before midnight and slowLoggingStop is after midnight
//    if (secondsUntilStop < 0) secondsUntilStop += 24 * 60 * 60; // Add a day's worth of seconds if required to make secondsUntilStop positive
//
//    if (secondsUntilStop < settings.slowLoggingIntervalSeconds) // If we need to sleep for less than slowLoggingIntervalSeconds
//      msToSleep = (secondsUntilStop + 1) * 1000UL; // Adjust msToSleep, adding one extra second to make sure the next wake is > slowLoggingStop
//  }
//  else // checkSleepOnUsBetweenReadings
//  {
//    msToSleep = (uint32_t)(settings.usBetweenReadings / 1000ULL); // Sleep for usBetweenReadings
//  }
//
//  uint32_t sysTicksToSleep;
//  if (msToSleep < 131000)
//  {
//    sysTicksToSleep = msToSleep * 32768L; // Do the multiply first for short intervals
//    sysTicksToSleep = sysTicksToSleep / 1000L; // Now do the divide
//  }
//  else
//  {
//    sysTicksToSleep = msToSleep / 1000L; // Do the division first for long intervals (to avoid an overflow)
//    sysTicksToSleep = sysTicksToSleep * 32768L; // Now do the multiply
//  }
//
//  return (sysTicksToSleep);
//}

//bool checkIfItIsTimeToSleep(void)
//{
//
//  if (checkSleepOnUsBetweenReadings()
//  || checkSleepOnRTCTime()
//  || checkSleepOnFastSlowPin())
//    return(true);
//  else
//    return(false);
//}

//Go to sleep if the time between readings is greater than maxUsBeforeSleep (2 seconds) and triggering is not enabled
//bool checkSleepOnUsBetweenReadings(void)
//{
//  if ((settings.useGPIO11ForTrigger == false) && (settings.usBetweenReadings >= maxUsBeforeSleep))
//    return (true);
//  else
//    return (false);
//}

//Go to sleep if Fast/Slow logging on Pin 11 is enabled and Pin 11 is in the correct state
//bool checkSleepOnFastSlowPin(void)
//{
//  if ((settings.useGPIO11ForFastSlowLogging == true) && (digitalRead(PIN_TRIGGER) == settings.slowLoggingWhenPin11Is))
//    return (true);
//  else
//    return (false);
//}

// Go to sleep if useRTCForFastSlowLogging is enabled and RTC time is between the start and stop times
//bool checkSleepOnRTCTime(void)
//{
//  // Check if we should be sleeping based on useGPIO11ForFastSlowLogging and slowLoggingStartMOD + slowLoggingStopMOD
//  bool sleepOnRTCTime = false;
//  if (settings.useRTCForFastSlowLogging == true)
//  {
//    if (settings.slowLoggingStartMOD != settings.slowLoggingStopMOD) // Only perform the check if the start and stop times are not equal
//    {
//      myRTC.getTime(); // Get the RTC time
//      int minutesOfDay = (myRTC.hour * 60) + myRTC.minute;
//
//      if (settings.slowLoggingStartMOD > settings.slowLoggingStopMOD) // If slow logging starts later than the stop time (i.e. slow over midnight)
//      {
//        if ((minutesOfDay >= settings.slowLoggingStartMOD) || (minutesOfDay < settings.slowLoggingStopMOD))
//          sleepOnRTCTime = true;
//      }
//      else // Slow logging starts earlier than the stop time
//      {
//        if ((minutesOfDay >= settings.slowLoggingStartMOD) && (minutesOfDay < settings.slowLoggingStopMOD))
//          sleepOnRTCTime = true;
//      }
//    }
//  }
//  return(sleepOnRTCTime);
//}

// gfvalvo's flash string helper code: https://forum.arduino.cc/index.php?topic=533118.msg3634809#msg3634809

void SerialPrint(const char *line)
{
  DoSerialPrint([](const char *ptr) {return *ptr;}, line);
}

void SerialPrint(const __FlashStringHelper *line)
{
  DoSerialPrint([](const char *ptr) {return (char) pgm_read_byte_near(ptr);}, (const char*) line);
}

void SerialPrintln(const char *line)
{
  DoSerialPrint([](const char *ptr) {return *ptr;}, line, true);
}

void SerialPrintln(const __FlashStringHelper *line)
{
  DoSerialPrint([](const char *ptr) {return (char) pgm_read_byte_near(ptr);}, (const char*) line, true);
}

void DoSerialPrint(char (*funct)(const char *), const char *string, bool newLine)
{
  char ch;

  while ((ch = funct(string++)))
  {
    Serial.print(ch);
    if (settings.useTxRxPinsForTerminal == true)
      Serial1.print(ch);
  }

  if (newLine)
  {
    Serial.println();
    if (settings.useTxRxPinsForTerminal == true)
      Serial1.println();
  }
}