static uint8_t  rxbuf[5];
static uint8_t  rxidx = 0;
bool newPacket = false;

void pollUnixPacket() {

  newPacket = false;

  // Non-blocking: read whatever bytes are in the buffer
  while (Serial1.available() > 0) {
    int b = Serial1.read();
    if (b < 0) break;

    rxbuf[rxidx++] = (uint8_t)b;

    if (rxidx == 5) {
      // --- A complete packet has arrived ---
      uint32_t unixTime =  (uint32_t)rxbuf[0]
                         | ((uint32_t)rxbuf[1] << 8)
                         | ((uint32_t)rxbuf[2] << 16)
                         | ((uint32_t)rxbuf[3] << 24);
      uint8_t hundredths = rxbuf[4];

      syncPacket.unix = unixTime;
      syncPacket.hundredths = hundredths;
      syncPacket.valid = true;
      newPacket = true;

      rxidx = 0; // reset for next packet
      break; // stop after one packet
    }
  }

  // Debug lines
  // Serial.print("UNIX time: ");
  // Serial.print(syncPacket.unix);
  // Serial.print(" | Hundredths: ");
  // Serial.println(syncPacket.hundredths);

  // If no new complete packet this cycle
  if (!newPacket) {
    syncPacket.valid = false;
  }
}

//Query each enabled sensor for its most recent data
void getData()
{
  measurementCount++;
  measurementTotal++;

  outputDataCount = 0; // Counter for binary writes to outputData

  if (online.IMU)
  {
    if (myICM.dataReady())
    {
      // commended by Sami //intPeriod.full = period; // Average period relative to extTime
      intPeriod.full = lastSamplingPeriod; // Sami: measured sampling period

      digitalWrite(PIN_STAT_LED, HIGH); // Turn on blue LED
      myICM.getAGMT(); //Update values
    }
  }

  delayMicroseconds(1000); // Wait until syncPacket data is available (delay may not be optimal)
  pollUnixPacket();

  // // Update RTC from master if more than RTC_UPDATE_INTERVAL_US has elapsed. 
  // if ((syncPacket.valid == true) && (elapsedMinutes(myRTC.minute, lastRTCSetMinutes) > RTC_UPDATE_INTERVAL_MINS)) {
  //   myRTC.setTime(syncPacket.hundredths, syncPacket.seconds, syncPacket.minutes, syncPacket.hours, syncPacket.days, syncPacket.months, syncPacket.years); 
  //   lastRTCSetMinutes = myRTC.minute;   
  // }

  // Read battery voltage and get top 8 bits
  batteryVoltage = (uint8_t)(analogRead(PIN_VIN_MONITOR) >> 6); 
  
//    Debug lines
//    Serial.print(syncPacket.years);
//    Serial.print(" ");
//    Serial.print(syncPacket.months);
//    Serial.print(" ");
//    Serial.print(syncPacket.days);
//    Serial.print(" ");
//    Serial.print(syncPacket.hours);
//    Serial.print(" ");
//    Serial.print(syncPacket.minutes); 
//    Serial.print(" ");           
//    Serial.print(syncPacket.seconds);
//    Serial.print(" ");
//    Serial.print(syncPacket.hundredths);
//    Serial.print(" : ");    
//    Serial.print(myRTC.year);
//    Serial.print(" ");
//    Serial.print(myRTC.month);
//    Serial.print(" ");
//    Serial.print(myRTC.dayOfMonth);
//    Serial.print(" ");
//    Serial.print(myRTC.hour);
//    Serial.print(" ");
//    Serial.print(myRTC.minute); 
//    Serial.print(" ");           
//    Serial.print(myRTC.seconds);
//    Serial.print(" ");
//    Serial.print(myRTC.hundredths);
//    Serial.print(" ");    
//    Serial.println(batteryVoltage);

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
  outputData[outputDataCount++] = (uint8_t)(syncPacket.valid);

  // 4 bytes for UNIX time (LSB first)
  outputData[outputDataCount++] = (uint8_t)(syncPacket.unix & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((syncPacket.unix >> 8) & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((syncPacket.unix >> 16) & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((syncPacket.unix >> 24) & 0xFF);

  outputData[outputDataCount++] = (uint8_t)(syncPacket.hundredths); 
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
  digitalWrite(PIN_STAT_LED, LOW); // Turn off the blue LED  
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

//uint32_t elapsedMicros(uint32_t now, uint32_t then)
//{
//  uint32_t diff;
//  
//  // Allow for 32-bit rollover
//  if (now < then) {
//    diff = (0xffff - then) + now;
//  } else {
//    diff = now - then;    
//  }
//  return diff;
//}

// OSCAR calculate the number of minutes between two
// numbers which are assumed to represent cuurent and  
// previous minute times in the range 0 to 59. Rollover
// detection is performed.
 
uint8_t elapsedMinutes(uint8_t current, uint8_t previous)
{
  uint8_t diff = 0; // difference between two readings in minutes

  if ((current < 60) && (previous < 60)) { // Bounds check
    if (current < previous) {
      diff = (60 - previous) + current; // Rollover has occurred
    } else {
      diff = current - previous; // Rollover has not occurred  
    }
  }
  
  return diff;
}
