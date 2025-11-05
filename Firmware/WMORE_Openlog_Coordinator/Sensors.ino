


#include "Sensors.h"

//Query each enabled sensor for its most recent data
// void getData(char* sdOutputData, size_t lenData)

// ----------------------------------------------------------------------------
// WMORE - pretty much completely custom
void getData() // WMORE - backwards compatibility with OLAv2.3
{
  measurementCount++;
  measurementTotal++;

  outputData[0] = '\0'; //Clear string contents
  outputDataCount = 0; // WMORE - Counter for binary writes to outputData

  if (online.IMU)
  {
    if (myICM.dataReady())
    {
      //localSampleTime.full = am_hal_stimer_counter_get(); // Local time measured by STIMER
      intPeriod.full = period; // Average period relative to extTime
      digitalWrite(PIN_STAT_LED, HIGH);
      myICM.getAGMT(); //Update values
      digitalWrite(PIN_STAT_LED, LOW);
    }
  }

  if (Serial1.available() == 7) { // A timestamp is available
    syncPacket.valid = true;
    syncPacket.years = Serial1.read();
    syncPacket.months = Serial1.read();
    syncPacket.days = Serial1.read();
    syncPacket.hours = Serial1.read(); 
    syncPacket.minutes = Serial1.read();
    syncPacket.seconds = Serial1.read();
    syncPacket.hundredths = Serial1.read();
  } else {  // Timestamp not available
    syncPacket.valid = false;
    syncPacket.years = 0;
    syncPacket.months = 0;
    syncPacket.days = 0;
    syncPacket.hours = 0; 
    syncPacket.minutes = 0;
    syncPacket.seconds = 0;
    syncPacket.hundredths = 0;
  }
  serialClearBuffer(1); // Get rid of any spurious characters

  // Read battery voltage and get top 8 bits
  batteryVoltage = (uint8_t)(analogRead(PIN_VIN_MONITOR) >> 6); 

  // Binary write assuming LSB first (ARM little-endian)
  outputData[outputDataCount++] = (uint8_t)(myICM.agmt.acc.axes.x & 0xFF); // LSB
  outputData[outputDataCount++] = (uint8_t)((myICM.agmt.acc.axes.x >> 8) & 0xFF); // MSB 
  outputData[outputDataCount++] = (uint8_t)(myICM.agmt.acc.axes.y & 0xFF); 
  outputData[outputDataCount++] = (uint8_t)((myICM.agmt.acc.axes.y >> 8) & 0xFF);
  outputData[outputDataCount++] = (uint8_t)(myICM.agmt.acc.axes.z & 0xFF); 
  outputData[outputDataCount++] = (uint8_t)((myICM.agmt.acc.axes.z >> 8) & 0xFF); 
  outputData[outputDataCount++] = (uint8_t)(myICM.agmt.gyr.axes.x & 0xFF); 
  outputData[outputDataCount++] = (uint8_t)((myICM.agmt.gyr.axes.x >> 8) & 0xFF); 
  outputData[outputDataCount++] = (uint8_t)(myICM.agmt.gyr.axes.y & 0xFF); 
  outputData[outputDataCount++] = (uint8_t)((myICM.agmt.gyr.axes.y >> 8) & 0xFF);  
  outputData[outputDataCount++] = (uint8_t)(myICM.agmt.gyr.axes.z & 0xFF); 
  outputData[outputDataCount++] = (uint8_t)((myICM.agmt.gyr.axes.z >> 8) & 0xFF); 
  outputData[outputDataCount++] = (uint8_t)(myICM.agmt.mag.axes.x & 0xFF); 
  outputData[outputDataCount++] = (uint8_t)((myICM.agmt.mag.axes.x >> 8) & 0xFF); 
  outputData[outputDataCount++] = (uint8_t)(myICM.agmt.mag.axes.y & 0xFF); 
  outputData[outputDataCount++] = (uint8_t)((myICM.agmt.mag.axes.y >> 8) & 0xFF); 
  outputData[outputDataCount++] = (uint8_t)(myICM.agmt.mag.axes.z & 0xFF); 
  outputData[outputDataCount++] = (uint8_t)((myICM.agmt.mag.axes.z >> 8) & 0xFF);
  outputData[outputDataCount++] = (uint8_t)(myICM.agmt.tmp.val & 0xFF); 
  outputData[outputDataCount++] = (uint8_t)((myICM.agmt.tmp.val >> 8) & 0xFF);                  
  outputData[outputDataCount++] = (uint8_t)(syncPacket.valid); // LSB
  outputData[outputDataCount++] = (uint8_t)(syncPacket.years); // MSB
  outputData[outputDataCount++] = (uint8_t)(syncPacket.months); // LSB
  outputData[outputDataCount++] = (uint8_t)(syncPacket.days); // MSB 
  outputData[outputDataCount++] = (uint8_t)(syncPacket.hours); // LSB
  outputData[outputDataCount++] = (uint8_t)(syncPacket.minutes); // MSB
  outputData[outputDataCount++] = (uint8_t)(syncPacket.seconds); // LSB
  outputData[outputDataCount++] = (uint8_t)(syncPacket.hundredths); // MSB   
  outputData[outputDataCount++] = (uint8_t)(myRTC.year); // LSB
  outputData[outputDataCount++] = (uint8_t)(myRTC.month); // MSB
  outputData[outputDataCount++] = (uint8_t)(myRTC.dayOfMonth); // LSB 
  outputData[outputDataCount++] = (uint8_t)(myRTC.hour); // MSB
  outputData[outputDataCount++] = (uint8_t)(myRTC.minute); // LSB
  outputData[outputDataCount++] = (uint8_t)(myRTC.seconds); // MSB
  outputData[outputDataCount++] = (uint8_t)(myRTC.hundredths); // LSB
  outputData[outputDataCount++] = (uint8_t)(batteryVoltage); // MSB
  outputData[outputDataCount++] = (uint8_t)(intPeriod.part[0]); // LSB
  outputData[outputDataCount++] = (uint8_t)(intPeriod.part[1]); // 
  outputData[outputDataCount++] = (uint8_t)(intPeriod.part[2]); // 
  outputData[outputDataCount++] = (uint8_t)(intPeriod.part[3]); // MSB                    
  totalCharactersPrinted += outputDataCount;
}

void printHelperText(uint8_t outputDest)
{
  // WMORE - was in misc.ino file and is blank. HELPER_BUFFER_SIZE is 1024 in OLAv2.10
  // char helperText[HELPER_BUFFER_SIZE];
  // helperText[0] = '\0';

  // getHelperText(helperText, sizeof(helperText));

  // if(outputDest & OL_OUTPUT_SERIAL)
  //   SerialPrint(helperText);

  // if ((outputDest & OL_OUTPUT_SDCARD) && (settings.logData == true) && (online.microSD))
  //   sensorDataFile.print(helperText);
}

//Read the VIN voltage
float readVIN()
{
  // Only supported on >= V10 hardware
#if(HARDWARE_VERSION_MAJOR == 0)
  return(0.0); // Return 0.0V on old hardware
#else
  int div3 = analogRead(PIN_VIN_MONITOR); //Read VIN across a 1/3 resistor divider
  float vin = (float)div3 * 3.0 * 2.0 / 16384.0; //Convert 1/3 VIN to VIN (14-bit resolution)
  vin = vin * settings.vinCorrectionFactor; //Correct for divider impedance (determined experimentally)
  return (vin);
#endif
}
