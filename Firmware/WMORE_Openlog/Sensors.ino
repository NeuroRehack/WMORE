#include "Sensors.h"
#include <time.h>

static uint8_t  rxbuf[5];
static uint8_t  rxidx = 0;
bool newPacket = false;

UnixTimeWithHunds local_unixTime;

// Describe what we receive over UART from the coordinator/logger
struct SyncPacket {
  uint32_t unix;        // UNIX time (seconds since 1970)
  uint8_t  hundredths;  // hundredths of a second
  bool     valid;       // true if a complete packet was received this cycle
  bool     isCoordinator; // true if this packet came from the coordinator
};

static SyncPacket syncPacket;  // <-- this is the global instance

UnixTimeWithHunds getUnixTimeFromRTC(void) {
  UnixTimeWithHunds result;
  struct tm t = {0};

  t.tm_year = myRTC.year + 100;
  t.tm_mon  = myRTC.month - 1;
  t.tm_mday = myRTC.dayOfMonth;
  t.tm_hour = myRTC.hour;
  t.tm_min  = myRTC.minute;
  t.tm_sec  = myRTC.seconds;
  t.tm_isdst = -1;

  result.unix = (uint32_t)mktime(&t);
  result.hundredths = myRTC.hundredths;
  return result;
}

// Convert UNIX time (seconds since 1970-01-01) to date/time in the range 2000–2099.
// Outputs: sec, min, hour, day (1–31), month (1–12), year2 (00–99 for 2000–2099).
bool unixToRTC(uint32_t unixTime,
               uint8_t &sec, uint8_t &min, uint8_t &hour,
               uint8_t &day, uint8_t &month, uint8_t &year2)
{
  // Seconds between 1970-01-01 and 2000-01-01 (inclusive of leap years)
  const uint32_t UNIX_2000 = 946684800UL;
  const uint32_t SECS_PER_DAY = 86400UL;

  if (unixTime < UNIX_2000) {
    return false; // out of supported range
  }

  uint32_t t = unixTime - UNIX_2000;

  // Time of day
  uint32_t days = t / SECS_PER_DAY;
  uint32_t sod  = t % SECS_PER_DAY;

  hour = sod / 3600;
  sod %= 3600;
  min  = sod / 60;
  sec  = sod % 60;

  // Date
  uint16_t year = 2000;
  auto isLeap = [](uint16_t y) {
    return (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
  };

  while (true) {
    uint16_t dYear = isLeap(year) ? 366 : 365;
    if (days >= dYear) {
      days -= dYear;
      year++;
    } else {
      break;
    }
  }

  static const uint8_t daysInMonthBase[12] = {
    31,28,31,30,31,30,31,31,30,31,30,31
  };

  month = 1;
  for (int i = 0; i < 12; ++i) {
    uint8_t dMonth = daysInMonthBase[i];
    if (i == 1 && isLeap(year)) {
      dMonth = 29; // February in leap year
    }
    if (days >= dMonth) {
      days -= dMonth;
      month++;
    } else {
      break;
    }
  }

  day   = days + 1;
  year2 = (uint8_t)(year - 2000); // convert to two-digit year (e.g. 25 for 2025)

  return true;
}

// Process Unix time package
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

      syncPacket.unix         = unixTime;
      syncPacket.hundredths   = hundredths;
      syncPacket.isCoordinator = (hundredths == 0xFF);  // 0xFF = special coordinator marker
      syncPacket.valid        = true;
      newPacket               = true;

      rxidx = 0; // reset for next packet
      break;     // stop after one packet
    }
  }

  // Debug lines
  Serial.print("UNIX time: ");
  Serial.print(syncPacket.unix);
  Serial.print(" | Hundredths: ");
  Serial.print(syncPacket.hundredths);
  Serial.print(" | isCoord: ");
  Serial.println(syncPacket.isCoordinator ? "YES" : "NO");

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

  // Get global UNIX time
  pollUnixPacket();

  // Get local UNIX time
  local_unixTime = getUnixTimeFromRTC();

  // Update RTC from master if more than RTC_UPDATE_INTERVAL_MINS has elapsed, 
  // packet is valid, not from coordinator, and UNIX time is non-zero.
  if (syncPacket.valid &&
      !syncPacket.isCoordinator &&
      (syncPacket.unix != 0) &&
      (elapsedMinutes(myRTC.minute, lastRTCSetMinutes) > RTC_UPDATE_INTERVAL_MINS))
  {
    uint8_t sec, min, hour, day, month, year2;

    if (unixToRTC(syncPacket.unix, sec, min, hour, day, month, year2)) {
      myRTC.setTime(syncPacket.hundredths,  // hundredths
                    sec,
                    min,
                    hour,
                    day,
                    month,
                    year2);

      lastRTCSetMinutes = myRTC.minute;
    }
  }

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
  // outputData[outputDataCount++] = (uint8_t)(myICM.agmt.tmp.val & 0xFF); 
  // outputData[outputDataCount++] = (uint8_t)((myICM.agmt.tmp.val >> 8) & 0xFF);                  
  outputData[outputDataCount++] = (uint8_t)(syncPacket.valid);

  // 4 bytes for global UNIX time (LSB first)
  outputData[outputDataCount++] = (uint8_t)(syncPacket.unix & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((syncPacket.unix >> 8) & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((syncPacket.unix >> 16) & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((syncPacket.unix >> 24) & 0xFF);

  outputData[outputDataCount++] = (uint8_t)(syncPacket.hundredths); 

  // 4 bytes for local UNIX time (LSB first)
  outputData[outputDataCount++] = (uint8_t)(local_unixTime.unix & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((local_unixTime.unix >> 8) & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((local_unixTime.unix >> 16) & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((local_unixTime.unix >> 24) & 0xFF);

  outputData[outputDataCount++] = (uint8_t)(local_unixTime.hundredths); // LSB

  outputData[outputDataCount++] = (uint8_t)(batteryVoltage); // MSB
  // outputData[outputDataCount++] = (uint8_t)(intPeriod.part[0]); // LSB
  // outputData[outputDataCount++] = (uint8_t)(intPeriod.part[1]); // 
  // outputData[outputDataCount++] = (uint8_t)(intPeriod.part[2]); // 
  // outputData[outputDataCount++] = (uint8_t)(intPeriod.part[3]); // MSB                    
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
