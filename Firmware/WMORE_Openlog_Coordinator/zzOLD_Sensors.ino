//Query each enabled sensor for its most recent data
void getData()
{
  measurementCount++;
  measurementTotal++;

  outputData[0] = '\0'; //Clear string contents
  outputDataCount = 0; // Counter for binary writes to outputData

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
  outputData[outputDataCount++] = (uint8_t)(syncPacket.years); // 
  outputData[outputDataCount++] = (uint8_t)(syncPacket.months); // 
  outputData[outputDataCount++] = (uint8_t)(syncPacket.days); // MSB 
  outputData[outputDataCount++] = (uint8_t)(syncPacket.hours); // LSB
  outputData[outputDataCount++] = (uint8_t)(syncPacket.minutes); // 
  outputData[outputDataCount++] = (uint8_t)(syncPacket.seconds); // 
  outputData[outputDataCount++] = (uint8_t)(syncPacket.hundredths); // MSB   
  outputData[outputDataCount++] = (uint8_t)(myRTC.year); // 
  outputData[outputDataCount++] = (uint8_t)(myRTC.month); // 
  outputData[outputDataCount++] = (uint8_t)(myRTC.dayOfMonth); // 
  outputData[outputDataCount++] = (uint8_t)(myRTC.hour); // 
  outputData[outputDataCount++] = (uint8_t)(myRTC.minute); // 
  outputData[outputDataCount++] = (uint8_t)(myRTC.seconds); // 
  outputData[outputDataCount++] = (uint8_t)(myRTC.hundredths); // 
  outputData[outputDataCount++] = (uint8_t)(batteryVoltage); // 
  outputData[outputDataCount++] = (uint8_t)(intPeriod.part[0]); // LSB
  outputData[outputDataCount++] = (uint8_t)(intPeriod.part[1]); // 
  outputData[outputDataCount++] = (uint8_t)(intPeriod.part[2]); // 
  outputData[outputDataCount++] = (uint8_t)(intPeriod.part[3]); // MSB                    
  totalCharactersPrinted += outputDataCount;
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
