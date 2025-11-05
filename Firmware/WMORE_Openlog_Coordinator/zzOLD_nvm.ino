void loadSettings()
{
  //First load any settings from NVM
  //After, we'll load settings from config file if available
  //We'll then re-record settings so that the settings from the file over-rides internal NVM settings

  //Check to see if EEPROM is blank
  uint32_t testRead = 0;
  if (EEPROM.get(0, testRead) == 0xFFFFFFFF)
  {
    SerialPrintln(F("EEPROM is blank. Default settings applied"));
    recordSystemSettings(); //Record default settings to EEPROM and config file. At power on, settings are in default state
  }

  //Check that the current settings struct size matches what is stored in EEPROM
  //Misalignment happens when we add a new feature or setting
  int tempSize = 0;
  EEPROM.get(0, tempSize); //Load the sizeOfSettings
  if (tempSize != sizeof(settings))
  {
    SerialPrintln(F("Settings wrong size. Default settings applied"));
    recordSystemSettings(); //Record default settings to EEPROM and config file. At power on, settings are in default state
  }

  //Check that the olaIdentifier is correct
  //(It is possible for two different versions of the code to have the same sizeOfSettings - which causes problems!)
  int tempIdentifier = 0;
  EEPROM.get(sizeof(int), tempIdentifier); //Load the identifier from the EEPROM location after sizeOfSettings (int)
  if (tempIdentifier != OLA_IDENTIFIER)
  {
    SerialPrintln(F("Settings are not valid for this variant of the OLA. Default settings applied"));
    recordSystemSettings(); //Record default settings to EEPROM and config file. At power on, settings are in default state
  }

  //Read current settings
  EEPROM.get(0, settings);

  loadSystemSettingsFromFile(); //Load any settings from config file. This will over-write any pre-existing EEPROM settings.
  //Record these new settings to EEPROM and config file to be sure they are the same
  //(do this even if loadSystemSettingsFromFile returned false)
  recordSystemSettings();
}

//Record the current settings struct to EEPROM and then to config file
void recordSystemSettings()
{
  settings.sizeOfSettings = sizeof(settings);
  EEPROM.put(0, settings);
  recordSystemSettingsToFile();
}

//Export the current settings to a config file
void recordSystemSettingsToFile()
{
  if (online.microSD == true)
  {
    if (sd.exists("OLA_settings.txt"))
      sd.remove("OLA_settings.txt");

    #if SD_FAT_TYPE == 1
    File32 settingsFile;
    #elif SD_FAT_TYPE == 2
    ExFile settingsFile;
    #elif SD_FAT_TYPE == 3
    FsFile settingsFile;
    #else // SD_FAT_TYPE == 0
    File settingsFile;
    #endif  // SD_FAT_TYPE
    if (settingsFile.open("OLA_settings.txt", O_CREAT | O_APPEND | O_WRITE) == false)
    {
      SerialPrintln(F("Failed to create settings file"));
      return;
    }

    settingsFile.println("sizeOfSettings=" + (String)settings.sizeOfSettings);
    settingsFile.println("olaIdentifier=" + (String)settings.olaIdentifier);
    settingsFile.println("nextSerialLogNumber=" + (String)settings.nextSerialLogNumber);
    settingsFile.println("nextDataLogNumber=" + (String)settings.nextDataLogNumber);

    // Convert uint64_t to string
    // Based on printLLNumber by robtillaart
    // https://forum.arduino.cc/index.php?topic=143584.msg1519824#msg1519824
    char tempTimeRev[20]; // Char array to hold to usBetweenReadings (reversed order)
    char tempTime[20]; // Char array to hold to usBetweenReadings (correct order)
    uint64_t usBR = settings.usBetweenReadings;
    unsigned int i = 0;
    if (usBR == 0ULL) // if usBetweenReadings is zero, set tempTime to "0"
    {
      tempTime[0] = '0';
      tempTime[1] = 0;
    }
    else
    {
      while (usBR > 0)
      {
        tempTimeRev[i++] = (usBR % 10) + '0'; // divide by 10, convert the remainder to char
        usBR /= 10; // divide by 10
      }
      unsigned int j = 0;
      while (i > 0)
      {
        tempTime[j++] = tempTimeRev[--i]; // reverse the order
        tempTime[j] = 0; // mark the end with a NULL
      }
    }
    
    settingsFile.println("usBetweenReadings=" + (String)tempTime);

    settingsFile.println("logMaxRate=" + (String)settings.logMaxRate);
    settingsFile.println("enableRTC=" + (String)settings.enableRTC);
    settingsFile.println("enableIMU=" + (String)settings.enableIMU);
    settingsFile.println("enableTerminalOutput=" + (String)settings.enableTerminalOutput);
    settingsFile.println("logDate=" + (String)settings.logDate);
    settingsFile.println("logTime=" + (String)settings.logTime);
    settingsFile.println("logData=" + (String)settings.logData);
    settingsFile.println("logSerial=" + (String)settings.logSerial);
    settingsFile.println("logIMUAccel=" + (String)settings.logIMUAccel);
    settingsFile.println("logIMUGyro=" + (String)settings.logIMUGyro);
    settingsFile.println("logIMUMag=" + (String)settings.logIMUMag);
    settingsFile.println("logIMUTemp=" + (String)settings.logIMUTemp);
    settingsFile.println("logRTC=" + (String)settings.logRTC);
    settingsFile.println("logHertz=" + (String)settings.logHertz);
    settingsFile.println("correctForDST=" + (String)settings.correctForDST);
    settingsFile.println("dateStyle=" + (String)settings.dateStyle);
    settingsFile.println("hour24Style=" + (String)settings.hour24Style);
    settingsFile.println("serialTerminalBaudRate=" + (String)settings.serialTerminalBaudRate);
    settingsFile.println("serialLogBaudRate=" + (String)settings.serialLogBaudRate);
    settingsFile.println("showHelperText=" + (String)settings.showHelperText);
    settingsFile.println("logA11=" + (String)settings.logA11);
    settingsFile.println("logA12=" + (String)settings.logA12);
    settingsFile.println("logA13=" + (String)settings.logA13);
    settingsFile.println("logA32=" + (String)settings.logA32);
    settingsFile.println("logAnalogVoltages=" + (String)settings.logAnalogVoltages);
    settingsFile.print("localUTCOffset="); settingsFile.println(settings.localUTCOffset);
    settingsFile.println("printDebugMessages=" + (String)settings.printDebugMessages);
    settingsFile.println("powerDownQwiicBusBetweenReads=" + (String)settings.powerDownQwiicBusBetweenReads);
    settingsFile.println("qwiicBusMaxSpeed=" + (String)settings.qwiicBusMaxSpeed);
    settingsFile.println("qwiicBusPowerUpDelayMs=" + (String)settings.qwiicBusPowerUpDelayMs);
    settingsFile.println("printMeasurementCount=" + (String)settings.printMeasurementCount);
    settingsFile.println("enablePwrLedDuringSleep=" + (String)settings.enablePwrLedDuringSleep);
    settingsFile.println("logVIN=" + (String)settings.logVIN);
    settingsFile.println("openNewLogFilesAfter=" + (String)settings.openNewLogFilesAfter);
    settingsFile.print("vinCorrectionFactor="); settingsFile.println(settings.vinCorrectionFactor);
    settingsFile.println("useGPIO32ForStopLogging=" + (String)settings.useGPIO32ForStopLogging);
    settingsFile.println("qwiicBusPullUps=" + (String)settings.qwiicBusPullUps);
    settingsFile.println("outputSerial=" + (String)settings.outputSerial);
    settingsFile.println("zmodemStartDelay=" + (String)settings.zmodemStartDelay);
    settingsFile.println("enableLowBatteryDetection=" + (String)settings.enableLowBatteryDetection);
    settingsFile.print("lowBatteryThreshold="); settingsFile.println(settings.lowBatteryThreshold);
    settingsFile.println("frequentFileAccessTimestamps=" + (String)settings.frequentFileAccessTimestamps);
    settingsFile.println("useGPIO11ForTrigger=" + (String)settings.useGPIO11ForTrigger);
    settingsFile.println("fallingEdgeTrigger=" + (String)settings.fallingEdgeTrigger);
    settingsFile.println("imuAccDLPF=" + (String)settings.imuAccDLPF);
    settingsFile.println("imuGyroDLPF=" + (String)settings.imuGyroDLPF);
    settingsFile.println("imuAccFSS=" + (String)settings.imuAccFSS);
    settingsFile.println("imuAccDLPFBW=" + (String)settings.imuAccDLPFBW);
    settingsFile.println("imuGyroFSS=" + (String)settings.imuGyroFSS);
    settingsFile.println("imuGyroDLPFBW=" + (String)settings.imuGyroDLPFBW);
    settingsFile.println("logMicroseconds=" + (String)settings.logMicroseconds);
    settingsFile.println("useTxRxPinsForTerminal=" + (String)settings.useTxRxPinsForTerminal);
    settingsFile.println("timestampSerial=" + (String)settings.timestampSerial);
    settingsFile.println("timeStampToken=" + (String)settings.timeStampToken);
    settingsFile.println("useGPIO11ForFastSlowLogging=" + (String)settings.useGPIO11ForFastSlowLogging);
    settingsFile.println("slowLoggingWhenPin11Is=" + (String)settings.slowLoggingWhenPin11Is);
    settingsFile.println("useRTCForFastSlowLogging=" + (String)settings.useRTCForFastSlowLogging);
    settingsFile.println("slowLoggingIntervalSeconds=" + (String)settings.slowLoggingIntervalSeconds);
    settingsFile.println("slowLoggingStartMOD=" + (String)settings.slowLoggingStartMOD);
    settingsFile.println("slowLoggingStopMOD=" + (String)settings.slowLoggingStopMOD);
    settingsFile.println("resetOnZeroDeviceCount=" + (String)settings.resetOnZeroDeviceCount);
    settingsFile.println("imuUseDMP=" + (String)settings.imuUseDMP);
    settingsFile.println("imuLogDMPQuat6=" + (String)settings.imuLogDMPQuat6);
    settingsFile.println("imuLogDMPQuat9=" + (String)settings.imuLogDMPQuat9);
    settingsFile.println("imuLogDMPAccel=" + (String)settings.imuLogDMPAccel);
    settingsFile.println("imuLogDMPGyro=" + (String)settings.imuLogDMPGyro);
    settingsFile.println("imuLogDMPCpass=" + (String)settings.imuLogDMPCpass);
    settingsFile.println("minimumAwakeTimeMillis=" + (String)settings.minimumAwakeTimeMillis);
    settingsFile.println("identifyBioSensorHubs=" + (String)settings.identifyBioSensorHubs);
    settingsFile.println("serialTxRxDuringSleep=" + (String)settings.serialTxRxDuringSleep);
    settingsFile.println("printGNSSDebugMessages=" + (String)settings.printGNSSDebugMessages);
    settingsFile.println("serialNumber=" + (String)settings.serialNumber); // OW   
    updateDataFileAccess(&settingsFile); // Update the file access time & date
    settingsFile.close();
  }
}

//If a config file exists on the SD card, load them and overwrite the local settings
//Heavily based on ReadCsvFile from SdFat library
//Returns true if some settings were loaded from a file
//Returns false if a file was not opened/loaded
bool loadSystemSettingsFromFile()
{
  if (online.microSD == true)
  {
    if (sd.exists("OLA_settings.txt"))
    {
      SdFile settingsFile; //FAT32
      if (settingsFile.open("OLA_settings.txt", O_READ) == false)
      {
        SerialPrintln(F("Failed to open settings file"));
        return (false);
      }

      char line[60];
      int lineNumber = 0;

      while (settingsFile.available()) {
        int n = settingsFile.fgets(line, sizeof(line));
        if (n <= 0) {
          SerialPrintf2("Failed to read line %d from settings file\r\n", lineNumber);
        }
        else if (line[n - 1] != '\n' && n == (sizeof(line) - 1)) {
          SerialPrintf2("Settings line %d too long\r\n", lineNumber);
          if (lineNumber == 0)
          {
            //If we can't read the first line of the settings file, give up
            SerialPrintln(F("Giving up on settings file"));
            settingsFile.close();
            return (false);
          }
        }
        else if (parseLine(line) == false) {
          SerialPrintf3("Failed to parse line %d: %s\r\n", lineNumber, line);
          if (lineNumber == 0)
          {
            //If we can't read the first line of the settings file, give up
            SerialPrintln(F("Giving up on settings file"));
            settingsFile.close();
            return (false);
          }
        }

        lineNumber++;
      }

      //SerialPrintln(F("Config file read complete"));
      settingsFile.close();
      return (true);
    }
    else
    {
      SerialPrintln(F("No config file found. Using settings from EEPROM."));
      //The defaults of the struct will be recorded to a file later on.
      return (false);
    }
  }

  SerialPrintln(F("Config file read failed: SD offline"));
  return (false); //SD offline
}

// Check for extra characters in field or find minus sign.
char* skipSpace(char* str) {
  while (isspace(*str)) str++;
  return str;
}

//Convert a given line from file into a settingName and value
//Sets the setting if the name is known
bool parseLine(char* str) {
  char* ptr;

  // Set strtok start of line.
  str = strtok(str, "=");
  if (!str) return false;

  //Store this setting name
  char settingName[40];
  sprintf(settingName, "%s", str);

  //Move pointer to end of line
  str = strtok(nullptr, "\n");
  if (!str) return false;

  // Convert string to double.
  double d = strtod(str, &ptr);

  if (str == ptr || *skipSpace(ptr)) return false;

  // Get setting name
  if (strcmp(settingName, "sizeOfSettings") == 0)
  {
    //We may want to cause a factory reset from the settings file rather than the menu
    //If user sets sizeOfSettings to -1 in config file, OLA will factory reset
    if (d == -1)
    {
      EEPROM.erase();
      sd.remove("OLA_settings.txt");
      SerialPrintln(F("OpenLog Artemis has been factory reset. Freezing. Please restart and open terminal at 115200bps."));
      while (1);
    }

    //Check to see if this setting file is compatible with this version of OLA
    if (d != sizeof(settings))
      SerialPrintf3("Warning: Settings size is %d but current firmware expects %d. Attempting to use settings from file.\r\n", d, sizeof(settings));

  }
  else if (strcmp(settingName, "olaIdentifier") == 0)
    settings.olaIdentifier = d;
  else if (strcmp(settingName, "nextSerialLogNumber") == 0)
    settings.nextSerialLogNumber = d;
  else if (strcmp(settingName, "nextDataLogNumber") == 0)
    settings.nextDataLogNumber = d;
  else if (strcmp(settingName, "usBetweenReadings") == 0)
  {
    settings.usBetweenReadings = d;
  }
  else if (strcmp(settingName, "logMaxRate") == 0)
    settings.logMaxRate = d;
  else if (strcmp(settingName, "enableRTC") == 0)
    settings.enableRTC = d;
  else if (strcmp(settingName, "enableIMU") == 0)
    settings.enableIMU = d;
  else if (strcmp(settingName, "enableTerminalOutput") == 0)
    settings.enableTerminalOutput = d;
  else if (strcmp(settingName, "logDate") == 0)
    settings.logDate = d;
  else if (strcmp(settingName, "logTime") == 0)
    settings.logTime = d;
  else if (strcmp(settingName, "logData") == 0)
    settings.logData = d;
  else if (strcmp(settingName, "logSerial") == 0)
    settings.logSerial = d;
  else if (strcmp(settingName, "logIMUAccel") == 0)
    settings.logIMUAccel = d;
  else if (strcmp(settingName, "logIMUGyro") == 0)
    settings.logIMUGyro = d;
  else if (strcmp(settingName, "logIMUMag") == 0)
    settings.logIMUMag = d;
  else if (strcmp(settingName, "logIMUTemp") == 0)
    settings.logIMUTemp = d;
  else if (strcmp(settingName, "logRTC") == 0)
    settings.logRTC = d;
  else if (strcmp(settingName, "logHertz") == 0)
    settings.logHertz = d;
  else if (strcmp(settingName, "correctForDST") == 0)
    settings.correctForDST = d;
  else if (strcmp(settingName, "dateStyle") == 0)
    settings.dateStyle = d;
  else if (strcmp(settingName, "americanDateStyle") == 0) // Included for backward-compatibility
    settings.dateStyle = d;
  else if (strcmp(settingName, "hour24Style") == 0)
    settings.hour24Style = d;
  else if (strcmp(settingName, "serialTerminalBaudRate") == 0)
    settings.serialTerminalBaudRate = d;
  else if (strcmp(settingName, "serialLogBaudRate") == 0)
    settings.serialLogBaudRate = d;
  else if (strcmp(settingName, "showHelperText") == 0)
    settings.showHelperText = d;
  else if (strcmp(settingName, "logA11") == 0)
    settings.logA11 = d;
  else if (strcmp(settingName, "logA12") == 0)
    settings.logA12 = d;
  else if (strcmp(settingName, "logA13") == 0)
    settings.logA13 = d;
  else if (strcmp(settingName, "logA32") == 0)
    settings.logA32 = d;
  else if (strcmp(settingName, "logAnalogVoltages") == 0)
    settings.logAnalogVoltages = d;
  else if (strcmp(settingName, "localUTCOffset") == 0)
    settings.localUTCOffset = d;
  else if (strcmp(settingName, "printDebugMessages") == 0)
    settings.printDebugMessages = d;
  else if (strcmp(settingName, "powerDownQwiicBusBetweenReads") == 0)
    settings.powerDownQwiicBusBetweenReads = d;
  else if (strcmp(settingName, "qwiicBusMaxSpeed") == 0)
    settings.qwiicBusMaxSpeed = d;
  else if (strcmp(settingName, "qwiicBusPowerUpDelayMs") == 0)
    settings.qwiicBusPowerUpDelayMs = d;
  else if (strcmp(settingName, "printMeasurementCount") == 0)
    settings.printMeasurementCount = d;
  else if (strcmp(settingName, "enablePwrLedDuringSleep") == 0)
    settings.enablePwrLedDuringSleep = d;
  else if (strcmp(settingName, "logVIN") == 0)
    settings.logVIN = d;
  else if (strcmp(settingName, "openNewLogFilesAfter") == 0)
    settings.openNewLogFilesAfter = d;
  else if (strcmp(settingName, "vinCorrectionFactor") == 0)
    settings.vinCorrectionFactor = d;
  else if (strcmp(settingName, "useGPIO32ForStopLogging") == 0)
    settings.useGPIO32ForStopLogging = d;
  else if (strcmp(settingName, "qwiicBusPullUps") == 0)
    settings.qwiicBusPullUps = d;
  else if (strcmp(settingName, "outputSerial") == 0)
    settings.outputSerial = d;
  else if (strcmp(settingName, "zmodemStartDelay") == 0)
    settings.zmodemStartDelay = d;
  else if (strcmp(settingName, "enableLowBatteryDetection") == 0)
    settings.enableLowBatteryDetection = d;
  else if (strcmp(settingName, "lowBatteryThreshold") == 0)
    settings.lowBatteryThreshold = d;
  else if (strcmp(settingName, "frequentFileAccessTimestamps") == 0)
    settings.frequentFileAccessTimestamps = d;
  else if (strcmp(settingName, "useGPIO11ForTrigger") == 0)
    settings.useGPIO11ForTrigger = d;
  else if (strcmp(settingName, "fallingEdgeTrigger") == 0)
    settings.fallingEdgeTrigger = d;
  else if (strcmp(settingName, "imuAccDLPF") == 0)
    settings.imuAccDLPF = d;
  else if (strcmp(settingName, "imuGyroDLPF") == 0)
    settings.imuGyroDLPF = d;
  else if (strcmp(settingName, "imuAccFSS") == 0)
    settings.imuAccFSS = d;
  else if (strcmp(settingName, "imuAccDLPFBW") == 0)
    settings.imuAccDLPFBW = d;
  else if (strcmp(settingName, "imuGyroFSS") == 0)
    settings.imuGyroFSS = d;
  else if (strcmp(settingName, "imuGyroDLPFBW") == 0)
    settings.imuGyroDLPFBW = d;
  else if (strcmp(settingName, "logMicroseconds") == 0)
    settings.logMicroseconds = d;
  else if (strcmp(settingName, "useTxRxPinsForTerminal") == 0)
    settings.useTxRxPinsForTerminal = d;
  else if (strcmp(settingName, "timestampSerial") == 0)
    settings.timestampSerial = d;
  else if (strcmp(settingName, "timeStampToken") == 0)
    settings.timeStampToken = d;
  else if (strcmp(settingName, "useGPIO11ForFastSlowLogging") == 0)
    settings.useGPIO11ForFastSlowLogging = d;
  else if (strcmp(settingName, "slowLoggingWhenPin11Is") == 0)
    settings.slowLoggingWhenPin11Is = d;
  else if (strcmp(settingName, "useRTCForFastSlowLogging") == 0)
    settings.useRTCForFastSlowLogging = d;
  else if (strcmp(settingName, "slowLoggingIntervalSeconds") == 0)
    settings.slowLoggingIntervalSeconds = d;
  else if (strcmp(settingName, "slowLoggingStartMOD") == 0)
    settings.slowLoggingStartMOD = d;
  else if (strcmp(settingName, "slowLoggingStopMOD") == 0)
    settings.slowLoggingStopMOD = d;
  else if (strcmp(settingName, "resetOnZeroDeviceCount") == 0)
    settings.resetOnZeroDeviceCount = d;
  else if (strcmp(settingName, "imuUseDMP") == 0)
    settings.imuUseDMP = d;
  else if (strcmp(settingName, "imuLogDMPQuat6") == 0)
    settings.imuLogDMPQuat6 = d;
  else if (strcmp(settingName, "imuLogDMPQuat9") == 0)
    settings.imuLogDMPQuat9 = d;
  else if (strcmp(settingName, "imuLogDMPAccel") == 0)
    settings.imuLogDMPAccel = d;
  else if (strcmp(settingName, "imuLogDMPGyro") == 0)
    settings.imuLogDMPGyro = d;
  else if (strcmp(settingName, "imuLogDMPCpass") == 0)
    settings.imuLogDMPCpass = d;
  else if (strcmp(settingName, "minimumAwakeTimeMillis") == 0)
    settings.minimumAwakeTimeMillis = d;
  else if (strcmp(settingName, "identifyBioSensorHubs") == 0)
    settings.identifyBioSensorHubs = d;
  else if (strcmp(settingName, "serialTxRxDuringSleep") == 0)
    settings.serialTxRxDuringSleep = d;
  else if (strcmp(settingName, "printGNSSDebugMessages") == 0)
    settings.printGNSSDebugMessages = d;
  else if (strcmp(settingName, "serialNumber") == 0) // OW
    settings.serialNumber = d;  
  else
    {
      SerialPrintf2("Unknown setting %s. Ignoring...\r\n", settingName);
      return(false);
    }

  return (true);
}

//Export the current device settings to a config file
void recordDeviceSettingsToFile()
{
  if (online.microSD == true)
  {
    if (sd.exists("OLA_deviceSettings.txt"))
      sd.remove("OLA_deviceSettings.txt");

    #if SD_FAT_TYPE == 1
    File32 settingsFile;
    #elif SD_FAT_TYPE == 2
    ExFile settingsFile;
    #elif SD_FAT_TYPE == 3
    FsFile settingsFile;
    #else // SD_FAT_TYPE == 0
    File settingsFile;
    #endif  // SD_FAT_TYPE
    if (settingsFile.open("OLA_deviceSettings.txt", O_CREAT | O_APPEND | O_WRITE) == false)
    {
      SerialPrintln(F("Failed to create device settings file"));
      return;
    }

    //Step through the node list, recording each node's settings
    char base[75];
    node *temp = head;
    while (temp != NULL)
    {
      sprintf(base, "%s.%d.%d.%d.%d.", getDeviceName(temp->deviceType), temp->deviceType, temp->address, temp->muxAddress, temp->portNumber);
      SerialPrintf2("recordSettingsToFile Unknown device: %s\r\n", base);
      temp = temp->next;
    }
    updateDataFileAccess(&settingsFile); // Update the file access time & date
    settingsFile.close();
  }
}

//If a device config file exists on the SD card, load them and overwrite the local settings
//Heavily based on ReadCsvFile from SdFat library
//Returns true if some settings were loaded from a file
//Returns false if a file was not opened/loaded
bool loadDeviceSettingsFromFile()
{
  if (online.microSD == true)
  {
    if (sd.exists("OLA_deviceSettings.txt"))
    {
      #if SD_FAT_TYPE == 1
      File32 settingsFile;
      #elif SD_FAT_TYPE == 2
      ExFile settingsFile;
      #elif SD_FAT_TYPE == 3
      FsFile settingsFile;
      #else // SD_FAT_TYPE == 0
      File settingsFile;
      #endif  // SD_FAT_TYPE

      if (settingsFile.open("OLA_deviceSettings.txt", O_READ) == false)
      {
        SerialPrintln(F("Failed to open device settings file"));
        return (false);
      }

      char line[150];
      int lineNumber = 0;

      while (settingsFile.available()) {
        int n = settingsFile.fgets(line, sizeof(line));
        if (n <= 0) {
          SerialPrintf2("Failed to read line %d from settings file\r\n", lineNumber);
        }
        else if (line[n - 1] != '\n' && n == (sizeof(line) - 1)) {
          SerialPrintf2("Settings line %d too long\n", lineNumber);
        }
        else if (parseDeviceLine(line) == false) {
          SerialPrintf3("Failed to parse line %d: %s\r\n", lineNumber + 1, line);
        }

        lineNumber++;
      }

      settingsFile.close();
      return (true);
    }
    else
    {
      SerialPrintln(F("No device config file found. Creating one with device defaults."));
      recordDeviceSettingsToFile(); //Record the current settings to create the initial file
      return (false);
    }
  }

  SerialPrintln(F("Device config file read failed: SD offline"));
  return (false); //SD offline
}

//Convert a given line from device setting file into a settingName and value
//Immediately applies the setting to the appropriate node
bool parseDeviceLine(char* str) {
  char* ptr;

  // Set strtok start of line.
  str = strtok(str, "=");
  if (!str) return false;

  //Store this setting name
  char settingName[150];
  sprintf(settingName, "%s", str);

  //Move pointer to end of line
  str = strtok(nullptr, "\n");
  if (!str) return false;

  // Convert string to double.
  double d = strtod(str, &ptr);
  if (str == ptr || *skipSpace(ptr)) return false;

  //Break device setting into its constituent parts
  char deviceSettingName[50];
  deviceType_e deviceType;
  uint8_t address;
  uint8_t muxAddress;
  uint8_t portNumber;
  uint8_t count = 0;
  char *split = strtok(settingName, ".");
  while (split != NULL)
  {
    if (count == 0)
      ; //Do nothing. This is merely the human friendly device name
    else if (count == 1)
      deviceType = (deviceType_e)atoi(split);
    else if (count == 2)
      address = atoi(split);
    else if (count == 3)
      muxAddress = atoi(split);
    else if (count == 4)
      portNumber = atoi(split);
    else if (count == 5)
      sprintf(deviceSettingName, "%s", split);
    split = strtok(NULL, ".");
    count++;
  }

  if (count < 5)
  {
    SerialPrintf2("Incomplete setting: %s\r\n", settingName);
    return false;
  }

  //Find the device in the list that has this device type and address
  void *deviceConfigPtr = getConfigPointer(deviceType, address, muxAddress, portNumber);
  if (deviceConfigPtr == NULL)
  {
    //Setting in file found but no matching device on bus is available
  }
  else
  {
    SerialPrintf2("Unknown device type: %d\r\n", deviceType);
    SerialFlush();
  }
  return (true);
}
