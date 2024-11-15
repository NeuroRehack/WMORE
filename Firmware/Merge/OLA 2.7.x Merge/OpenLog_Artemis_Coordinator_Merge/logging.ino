//Print a message both to terminal and to log
void msg(const char * message)
{
  SerialPrintln(message);
  if (online.microSD)
    sensorDataFile.println(message);
}


// Added back in by Nathan // Merge
//----------------------------------------------------------------------------
// OW
// Generates file name in the form yymmdd_hhmmss_ID.bin
// Currently no file existence checks

char* generateFileName(void)
{
  static char newFileName[21]; // Declare storage for file name
  
  // Generate the filename
  // The following line should work but the sprintf library doesn't appear to generate leading zeroes correctly
  //sprintf(newFileName, "%02d%02d%02d_%02d%02d%02d.bin", myRTC.year, myRTC.month, myRTC.dayOfMonth, myRTC.hour, myRTC.minute, myRTC.seconds);
  // Workaround to address apparent failure of %02d format specifier to reliably add leading zeroes
  sprintf(newFileName, "%2s", dig2(myRTC.year));
  sprintf(newFileName, "%2s%2s", newFileName, dig2(myRTC.month));
  sprintf(newFileName, "%4s%2s_", newFileName, dig2(myRTC.dayOfMonth)); 
  sprintf(newFileName, "%7s%2s", newFileName, dig2(myRTC.hour));
  sprintf(newFileName, "%9s%2s", newFileName, dig2(myRTC.minute));  
  sprintf(newFileName, "%11s%2s_", newFileName, dig2(myRTC.seconds)); 
  sprintf(newFileName, "%14s%2s.bin", newFileName, dig2(settings.serialNumber));   
  
  // Tell the user
  //SerialPrint(F("Logging to: "));
  //SerialPrintln(newFileName);    

  return (newFileName);
}
// Added back in by Nathan // Merge
//----------------------------------------------------------------------------
// OW 
// Workaround for apparent non-compliance of %02d format specifer in sprintf().
// Returns two character string representation of int
// with leading zeroes if 0 < number < 100. 

char* dig2(int number)
{
  static char numberString[3];

  if(number < 10) 
  {
    // add leading zero 
    sprintf(numberString, "0%1d", number);      
  } 
  else 
  {
    if(number < 100) {
      // print two digits
      sprintf(numberString, "%2d", number);
    } 
    else 
    {
      // over/underflow
      sprintf(numberString, "**"); 
    }
  }

  return(numberString);
}

//Returns next available log file name
//Checks the spots in EEPROM for the next available LOG# file name
//Updates EEPROM and then appends to the new log file.
char* findNextAvailableLog(int &newFileNumber, const char *fileLeader)
{
  SdFile newFile; //This will contain the file for SD writing

  if (newFileNumber < 2) //If the settings have been reset, let's warn the user that this could take a while!
  {
    SerialPrintln(F("Finding the next available log file."));
    SerialPrintln(F("This could take a long time if the SD card contains many existing log files."));
  }

  if (newFileNumber > 0)
    newFileNumber--; //Check if last log file was empty. Reuse it if it is.

  //Search for next available log spot
  static char newFileName[40];
  while (1)
  {
    char newFileNumberStr[6];
    if (newFileNumber < 10)
      sprintf(newFileNumberStr, "0000%d", newFileNumber);
    else if (newFileNumber < 100)
      sprintf(newFileNumberStr, "000%d", newFileNumber);
    else if (newFileNumber < 1000)
      sprintf(newFileNumberStr, "00%d", newFileNumber);
    else if (newFileNumber < 10000)
      sprintf(newFileNumberStr, "0%d", newFileNumber);
    else
      sprintf(newFileNumberStr, "%d", newFileNumber);
    sprintf(newFileName, "%s%s.TXT", fileLeader, newFileNumberStr); //Splice the new file number into this file name. Max no. is 99999.

    if (sd.exists(newFileName) == false) break; //File name not found so we will use it.

    //File exists so open and see if it is empty. If so, use it.
    newFile.open(newFileName, O_READ);
    if (newFile.fileSize() == 0) break; // File is empty so we will use it. Note: we need to make the user aware that this can happen!

    newFile.close(); // Close this existing file we just opened.

    newFileNumber++; //Try the next number
    if (newFileNumber >= 100000) break; // Have we hit the maximum number of files?
  }
  
  newFile.close(); //Close this new file we just opened

  newFileNumber++; //Increment so the next power up uses the next file #

  if (newFileNumber >= 100000) // Have we hit the maximum number of files?
  {
    SerialPrint(F("***** WARNING! File number limit reached! (Overwriting "));
    SerialPrint(newFileName);
    SerialPrintln(F(") *****"));
    newFileNumber = 100000; // This will overwrite Log99999.TXT next time thanks to the newFileNumber-- above
  }
  else
  {
    SerialPrint(F("Logging to: "));
    SerialPrintln(newFileName);    
  }

  //Record new file number to EEPROM and to config file
  //This works because newFileNumber is a pointer to settings.newFileNumber
  recordSystemSettings();

  return (newFileName);
}
