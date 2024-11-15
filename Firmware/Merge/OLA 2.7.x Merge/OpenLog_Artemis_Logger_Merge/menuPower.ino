void menuPower()
{
  while (1)
  {
    SerialPrintln(F(""));
    SerialPrintln(F("Menu: Configure Power Options"));

    SerialPrint(F("1) Turn off Qwiic bus power between readings (>2s): "));
    if (settings.powerDownQwiicBusBetweenReads == true) SerialPrintln(F("Yes"));
    else SerialPrintln(F("No"));

    SerialPrint(F("2) Use pin 32 to Stop Logging: "));
    if (settings.useGPIO32ForStopLogging == true) SerialPrintln(F("Yes"));
    else SerialPrintln(F("No"));

#if(HARDWARE_VERSION_MAJOR >= 1)
    SerialPrint(F("3) Power LED During Sleep: "));
    if (settings.enablePwrLedDuringSleep == true) SerialPrintln(F("Enabled"));
    else SerialPrintln(F("Disabled"));

    SerialPrint(F("4) Low Battery Voltage Detection: "));
    if (settings.enableLowBatteryDetection == true) SerialPrintln(F("Enabled"));
    else SerialPrintln(F("Disabled"));

    SerialPrint(F("5) Low Battery Threshold (V): "));
    char tempStr[16];
    olaftoa(settings.lowBatteryThreshold, tempStr, 2, sizeof(tempStr) / sizeof(char));
    SerialPrintf2("%s\r\n", tempStr);

    SerialPrint(F("6) VIN measurement correction factor: "));
    olaftoa(settings.vinCorrectionFactor, tempStr, 3, sizeof(tempStr) / sizeof(char));
    SerialPrintf2("%s\r\n", tempStr);
#endif

    SerialPrint(F("7) Serial Tx and Rx pins during sleep are: "));
    if (settings.serialTxRxDuringSleep == true) SerialPrintln(F("Enabled"));
    else SerialPrintln(F("Disabled"));

    SerialPrintln(F("x) Exit"));

    byte incoming = getByteChoice(menuTimeout); //Timeout after x seconds

    if (incoming == '1')
    {
      settings.powerDownQwiicBusBetweenReads ^= 1;
    }
    else if (incoming == '2')
    {
      if (settings.identifyBioSensorHubs == false)
      {
        if (settings.useGPIO32ForStopLogging == true)
        {
          // Disable stop logging
          settings.useGPIO32ForStopLogging = false;
          detachInterrupt(PIN_STOP_LOGGING); // Disable the interrupt
          pinMode(PIN_STOP_LOGGING, INPUT); // Remove the pull-up
          pin_config(PinName(PIN_STOP_LOGGING), g_AM_HAL_GPIO_INPUT); // Make sure the pin does actually get re-configured
          stopLoggingSeen = false; // Make sure the flag is clear
        }
        else
        {
          // Enable stop logging
          settings.useGPIO32ForStopLogging = true;
          pinMode(PIN_STOP_LOGGING, INPUT_PULLUP);
          pin_config(PinName(PIN_STOP_LOGGING), g_AM_HAL_GPIO_INPUT_PULLUP); // Make sure the pin does actually get re-configured
          delay(1); // Let the pin stabilize
          attachInterrupt(PIN_STOP_LOGGING, stopLoggingISR, FALLING); // Enable the interrupt
          am_hal_gpio_pincfg_t intPinConfig = g_AM_HAL_GPIO_INPUT_PULLUP;
          intPinConfig.eIntDir = AM_HAL_GPIO_PIN_INTDIR_HI2LO;
          pin_config(PinName(PIN_STOP_LOGGING), intPinConfig); // Make sure the pull-up does actually stay enabled
          stopLoggingSeen = false; // Make sure the flag is clear
          settings.logA32 = false; // Disable analog logging on pin 32
        }
      }
      else
      {
        SerialPrintln(F(""));
        SerialPrintln(F("Stop logging via pin 32 is not possible. \"Detect Bio Sensor Pulse Oximeter\" is enabled."));
        SerialPrintln(F(""));
      }              
    }
#if(HARDWARE_VERSION_MAJOR >= 1)
    else if (incoming == '3')
    {
      settings.enablePwrLedDuringSleep ^= 1;
    }
    else if (incoming == '4')
    {
      settings.enableLowBatteryDetection ^= 1;
    }
    else if (incoming == '5')
    {
      SerialPrintln(F("Please enter the new low battery threshold:"));
      float tempBT = (float)getDouble(menuTimeout); //Timeout after x seconds
      if ((tempBT < 3.0) || (tempBT > 6.0))
        SerialPrintln(F("Error: Threshold out of range"));
      else
        settings.lowBatteryThreshold = tempBT;
    }
    else if (incoming == '6')
    {
      SerialPrintln(F("Please measure the voltage on the MEAS pin and enter it here:"));
      float tempCF = (float)getDouble(menuTimeout); //Timeout after x seconds
      int div3 = analogRead(PIN_VIN_MONITOR); //Read VIN across a 1/3 resistor divider
      float vin = (float)div3 * 3.0 * 2.0 / 16384.0; //Convert 1/3 VIN to VIN (14-bit resolution)
      tempCF = tempCF / vin; //Calculate the new correction factor
      if ((tempCF < 1.0) || (tempCF > 2.0))
        SerialPrintln(F("Error: Correction factor out of range"));
      else
        settings.vinCorrectionFactor = tempCF;
    }
#endif
    else if (incoming == '7')
    {
      settings.serialTxRxDuringSleep ^= 1;
    }
    else if (incoming == 'x')
      break;
    else if (incoming == STATUS_GETBYTE_TIMEOUT)
      break;
    else
      printUnknown(incoming);
  }
}
