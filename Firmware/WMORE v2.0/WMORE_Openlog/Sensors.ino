#include "Sensors.h"
#include <time.h>

// UART receive buffer for UNIX time packets (4 bytes UNIX + 1 byte hundredths)
static uint8_t  rxbuf[5];
static uint8_t  rxidx      = 0;
static bool     newPacket  = false;

UnixTimeWithHunds local_unixTime;

// Global SyncPacket instance (declared in Sensors.h)
SyncPacket g_syncPacket;

//------------------------------------------------------------------------------
// Local / global RTC helpers
//------------------------------------------------------------------------------

UnixTimeWithHunds getUnixTimeFromRTC(void)
{
  UnixTimeWithHunds result;
  struct tm t = {0};

  // myRTC.year is 0–99 for 2000–2099 -> tm_year is years since 1900
  t.tm_year  = myRTC.year + 100;       // 2000 = 100
  t.tm_mon   = myRTC.month - 1;        // tm_mon is 0–11
  t.tm_mday  = myRTC.dayOfMonth;
  t.tm_hour  = myRTC.hour;
  t.tm_min   = myRTC.minute;
  t.tm_sec   = myRTC.seconds;
  t.tm_isdst = -1;

  result.unix        = (uint32_t)mktime(&t);
  result.hundredths  = myRTC.hundredths;
  return result;
}

// Convert UNIX time (seconds since 1970-01-01) to date/time in the range 2000–2099.
bool unixToRTC(uint32_t unixTime,
               uint8_t &sec, uint8_t &min, uint8_t &hour,
               uint8_t &day, uint8_t &month, uint8_t &year2)
{
  const uint32_t UNIX_2000     = 946684800UL; // Seconds from 1970-01-01 to 2000-01-01
  const uint32_t SECS_PER_DAY  = 86400UL;

  if (unixTime < UNIX_2000) {
    return false; // out of supported range
  }

  uint32_t t    = unixTime - UNIX_2000;
  uint32_t days = t / SECS_PER_DAY;
  uint32_t sod  = t % SECS_PER_DAY;

  // Time of day
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
  year2 = (uint8_t)(year - 2000); // e.g. 2025 -> 25

  return true;
}

//------------------------------------------------------------------------------
// UART sync packet handling
//------------------------------------------------------------------------------

void pollUnixPacket(void)
{
  newPacket = false;

  // Non-blocking: read whatever bytes are in the buffer
  while (Serial1.available() > 0) {
    int b = Serial1.read();
    if (b < 0) break;

    rxbuf[rxidx++] = (uint8_t)b;

    if (rxidx == 5) {
      // A complete packet has arrived: 4 bytes UNIX + 1 byte hundredths
      uint32_t unixTime =
          (uint32_t)rxbuf[0]
        | ((uint32_t)rxbuf[1] << 8)
        | ((uint32_t)rxbuf[2] << 16)
        | ((uint32_t)rxbuf[3] << 24);
      uint8_t hundredths = rxbuf[4];

      g_syncPacket.unix          = unixTime;
      g_syncPacket.hundredths    = hundredths;
      g_syncPacket.isCoordinator = (hundredths == 0xFF); // 0xFF = special coordinator marker
      g_syncPacket.valid         = true;
      newPacket                  = true;

      rxidx = 0; // reset for next packet
      break;     // stop after one packet
    }
  }

  // Debug lines:
  Serial.print("UNIX time: ");
  Serial.print(g_syncPacket.unix);
  Serial.print(" | Hundredths: ");
  Serial.print(g_syncPacket.hundredths);
  Serial.print(" | isCoord: ");
  Serial.println(g_syncPacket.isCoordinator ? "YES" : "NO");


  // If no new complete packet this cycle, mark it invalid
  if (!newPacket) {
    g_syncPacket.valid = false;
  }
}

//------------------------------------------------------------------------------
// Main sensor readout
//------------------------------------------------------------------------------

void getData(void)
{
  measurementCount++;
  measurementTotal++;

  outputDataCount = 0; // Counter for binary writes to outputData

  // IMU read
  if (online.IMU) {
    if (myICM.dataReady()) {
      intPeriod.full = lastSamplingPeriod; // measured sampling period
      digitalWrite(PIN_STAT_LED, HIGH);    // Turn on blue LED
      myICM.getAGMT();                     // Update IMU values
    }
  }

  // Wait until syncPacket data is available (delay may not be optimal)
  delayMicroseconds(1000);

  // Get global UNIX time from coordinator (if this is the coordinator, it will read { 0x00, 0x00, 0x00, 0x00, 0xFF })
  pollUnixPacket();

  // Get local UNIX time
  local_unixTime = getUnixTimeFromRTC();

  // Update RTC from master if enough time has passed
  if (g_syncPacket.valid &&
      !g_syncPacket.isCoordinator &&
      (g_syncPacket.unix != 0) &&
      (elapsedMinutes(myRTC.minute, lastRTCSetMinutes) > RTC_UPDATE_INTERVAL_MINS))
  {
    uint8_t sec, min, hour, day, month, year2;

    if (unixToRTC(g_syncPacket.unix, sec, min, hour, day, month, year2)) {
      myRTC.setTime(g_syncPacket.hundredths,  // hundredths
                    sec,
                    min,
                    hour,
                    day,
                    month,
                    year2);

      lastRTCSetMinutes = myRTC.minute;
      // Debug lines:
      // Serial.println("Updated!");
    }
  }

  // Read battery voltage (top 8 bits)
  batteryVoltage = (uint8_t)(analogRead(PIN_VIN_MONITOR) >> 6); 

  // ---------------------------------------------------------------------------
  // Pack sensor data into outputData (little-endian)
  // ---------------------------------------------------------------------------

  // Accel
  outputData[outputDataCount++] = (uint8_t)(myICM.agmt.acc.axes.x & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((myICM.agmt.acc.axes.x >> 8) & 0xFF);
  outputData[outputDataCount++] = (uint8_t)(myICM.agmt.acc.axes.y & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((myICM.agmt.acc.axes.y >> 8) & 0xFF);
  outputData[outputDataCount++] = (uint8_t)(myICM.agmt.acc.axes.z & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((myICM.agmt.acc.axes.z >> 8) & 0xFF);

  // Gyro
  outputData[outputDataCount++] = (uint8_t)(myICM.agmt.gyr.axes.x & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((myICM.agmt.gyr.axes.x >> 8) & 0xFF);
  outputData[outputDataCount++] = (uint8_t)(myICM.agmt.gyr.axes.y & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((myICM.agmt.gyr.axes.y >> 8) & 0xFF);
  outputData[outputDataCount++] = (uint8_t)(myICM.agmt.gyr.axes.z & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((myICM.agmt.gyr.axes.z >> 8) & 0xFF);

  // Magnetometer
  outputData[outputDataCount++] = (uint8_t)(myICM.agmt.mag.axes.x & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((myICM.agmt.mag.axes.x >> 8) & 0xFF);
  outputData[outputDataCount++] = (uint8_t)(myICM.agmt.mag.axes.y & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((myICM.agmt.mag.axes.y >> 8) & 0xFF);
  outputData[outputDataCount++] = (uint8_t)(myICM.agmt.mag.axes.z & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((myICM.agmt.mag.axes.z >> 8) & 0xFF);

  // Sync packet status
  outputData[outputDataCount++] = (uint8_t)(g_syncPacket.valid);

  // 4 bytes for global UNIX time (LSB first)
  outputData[outputDataCount++] = (uint8_t)(g_syncPacket.unix & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((g_syncPacket.unix >> 8) & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((g_syncPacket.unix >> 16) & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((g_syncPacket.unix >> 24) & 0xFF);

  // Hundredths for global time
  outputData[outputDataCount++] = g_syncPacket.hundredths;

  // 4 bytes for local UNIX time (LSB first)
  outputData[outputDataCount++] = (uint8_t)(local_unixTime.unix & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((local_unixTime.unix >> 8) & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((local_unixTime.unix >> 16) & 0xFF);
  outputData[outputDataCount++] = (uint8_t)((local_unixTime.unix >> 24) & 0xFF);

  // Hundredths for local time
  outputData[outputDataCount++] = local_unixTime.hundredths;

  // Battery voltage (8-bit)
  outputData[outputDataCount++] = batteryVoltage;

  totalCharactersPrinted += outputDataCount;
  digitalWrite(PIN_STAT_LED, LOW); // Turn off the blue LED  
}

//------------------------------------------------------------------------------
// VIN and minute helpers
//------------------------------------------------------------------------------

float readVIN(void)
{
#if (HARDWARE_VERSION_MAJOR == 0)
  return 0.0f; // Return 0.0V on old hardware
#else
  int   div3 = analogRead(PIN_VIN_MONITOR);          // Read VIN across a 1/3 resistor divider
  float vin  = (float)div3 * 3.0f * 2.0f / 16384.0f; // Convert 1/3 VIN to VIN (14-bit resolution)
  vin *= settings.vinCorrectionFactor;               // Correct for divider impedance
  return vin;
#endif
}

// Calculate number of minutes between two readings (0–59) with rollover handling.
uint8_t elapsedMinutes(uint8_t current, uint8_t previous)
{
  uint8_t diff = 0;

  if ((current < 60) && (previous < 60)) {
    if (current < previous) {
      diff = (uint8_t)((60 - previous) + current); // Rollover has occurred
    } else {
      diff = (uint8_t)(current - previous);        // No rollover
    }
  }

  return diff;
}