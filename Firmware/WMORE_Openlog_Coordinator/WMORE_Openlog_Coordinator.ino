/*
  OpenLog Artemis
  By: Nathan Seidle
  SparkFun Electronics
  Date: November 26th, 2019
  License: This code is public domain but you buy me a beer if you use this
  and we meet someday (Beerware license).
  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/15793

  This firmware runs the OpenLog Artemis. A large variety of system settings can be
  adjusted by connecting at 115200bps.

  The Board should be set to SparkFun Apollo3 \ RedBoard Artemis ATP.

  v1.0 Power Consumption:
   Sleep between reads, RTC fully charged, no Qwiic, SD, no USB, no Power LED: 260uA
   10Hz logging IMU, no Qwiic, SD, no USB, no Power LED: 9-27mA

  TODO:
  (done) Create settings file for sensor. Load after qwiic bus is scanned.
  (done on larger Strings) Remove String dependencies.
  (done) Bubble sort list of devices.
  (done) Remove listing for muxes.
  (done) Verify the printing of all sensors is %f, %d correct
  (done) Add begin function seperate from everything, call after wakeup instead of detect
  (done) Add counter to output to look for memory leaks on long runs
  (done) Add AHT20 support
  (done) Add SHTC3 support
  (done) Change settings extension to txt
  (done) Fix max I2C speed to use linked list
  Currently device settings are not recorded to EEPROM, only deviceSettings.txt
  Is there a better way to dynamically create size of outputData array so we don't ever get larger than X sensors outputting?
  Find way to store device configs into EEPROM
  Log four pressure sensors and graph them on plotter
  (checked) Test GPS - not sure about %d with int32s. Does lat, long, and alt look correct?
  (done) Test NAU7802s
  (done) Test SCD30s (Add an extended delay for the SCD30. (Issue #5))
  (won't do?) Add a 'does not like to be powered cycled' setting for each device type. I think this has been superceded by "Add individual power-on delays for each sensor type?.
  (done) Add support for logging VIN
  (done) Investigate error in time between logs (https://github.com/sparkfun/OpenLog_Artemis/issues/13)
  (done) Invesigate RTC reset issue (https://github.com/sparkfun/OpenLog_Artemis/issues/13 + https://forum.sparkfun.com/viewtopic.php?f=123&t=53157)
    The solution is to make sure that the OLA goes into deep sleep as soon as the voltage monitor detects that the power has failed.
    The user will need to press the reset button once power has been restored. Using the WDT to check the monitor and do a POR wasn't reliable.
  (done) Investigate requires-reset issue on battery power (") (X04 + CCS811/BME280 enviro combo)
  (done) Add a fix so that the MS8607 does not also appear as an MS5637
  (done) Add "set RTC from GPS" functionality
  (done) Add UTCoffset functionality (including support for negative numbers)
  (done) Figure out how to give the u-blox time to establish a fix if it has been powered down between log intervals. The user can specify up to 60s for the Qwiic power-on delay.
  Add support for VREG_ENABLE
  (done) Add support for PWR_LED
  (done) Use the WDT to reset the Artemis when power is reconnected (previously the Artemis would have stayed in deep sleep)
  Add a callback function to the u-blox library so we can abort waiting for UBX data if the power goes low
  (done) Add support for the ADS122C04 ADC (Qwiic PT100)
  (done) Investigate why usBetweenReadings appears to be longer than expected. We needed to read millis _before_ enabling the lower power clock!
  (done) Correct u-blox pull-ups
  (done) Add an olaIdentifier to prevent problems when using two code variants that have the same sizeOfSettings
  (done) Add a fix for the IMU wake-up issue identified in https://github.com/sparkfun/OpenLog_Artemis/issues/18
  (done) Add a "stop logging" feature on GPIO 32: allow the pin to be used to read a stop logging button instead of being an analog input
  (done) Allow the user to set the default qwiic bus pull-up resistance (u-blox will still use 'none')
  (done) Add support for low battery monitoring using VIN
  (done) Output sensor data via the serial TX pin (Issue #32)
  (done) Add support for SD card file transfer (ZMODEM) and delete. (Issue #33) With thanks to: ecm-bitflipper (https://github.com/ecm-bitflipper/Arduino_ZModem)
  (done) Add file creation and access timestamps
  (done) Add the ability to trigger data collection via Pin 11 (Issue #36)
  (done) Correct the measurement count misbehaviour (Issue #31)
  (done) Use the corrected IMU temperature calculation (Issue #28)
  (done) Add individual power-on delays for each sensor type. Add an extended delay for the SCD30. (Issue #5)
  (done) v1.7: Fix readVin after sleep bug: https://github.com/sparkfun/OpenLog_Artemis/issues/39
  (done) Change detectQwiicDevices so that the MCP9600 (Qwiic Thermocouple) is detected correctly
  (done) Add support for the MPRLS0025PA micro pressure sensor
  (done) Add support for the SN-GCJA5 particle sensor
  (done) Add IMU accelerometer and gyro full scale and digital low pass filter settings to menuIMU
  (done) Add a fix to make sure the MS8607 is detected correctly: https://github.com/sparkfun/OpenLog_Artemis/issues/54
  (done) Add logMicroseconds: https://github.com/sparkfun/OpenLog_Artemis/issues/49
  (done) Add an option to use autoPVT when logging GNSS data: https://github.com/sparkfun/OpenLog_Artemis/issues/50
  (done) Corrected an issue when using multiple MS8607's: https://github.com/sparkfun/OpenLog_Artemis/issues/62
  (done) Add a feature to use the TX and RX pins as a duplicate Terminal
  (done) Add serial log timestamps with a token (as suggested by @DennisMelamed in PR https://github.com/sparkfun/OpenLog_Artemis/pull/70 and Issue https://github.com/sparkfun/OpenLog_Artemis/issues/63)
  (done) Add "sleep on pin" functionality based @ryanneve's PR https://github.com/sparkfun/OpenLog_Artemis/pull/64 and Issue https://github.com/sparkfun/OpenLog_Artemis/issues/46
  (done) Add "wake at specified times" functionality based on Issue https://github.com/sparkfun/OpenLog_Artemis/issues/46
  (done) Add corrections for the SCD30 based on Forum post by paulvha: https://forum.sparkfun.com/viewtopic.php?p=222455#p222455
  (done) Add support for the SGP40 VOC Index sensor
  (done) Add support for the SDP3X Differential Pressure sensor
  (done) Add support for the MS5837 - as used in the BlueRobotics BAR02 and BAR30 water pressure sensors
  (done) Correct an issue which was causing the OLA to crash when waking from sleep and outputting serial data https://github.com/sparkfun/OpenLog_Artemis/issues/79
  (done) Correct low-power code as per https://github.com/sparkfun/OpenLog_Artemis/issues/78
  (done) Correct a bug in menuAttachedDevices when useTxRxPinsForTerminal is enabled https://github.com/sparkfun/OpenLog_Artemis/issues/82
  (done) Add ICM-20948 DMP support. Requires v1.2.6 of the ICM-20948 library. DMP logging is limited to: Quat6 or Quat9, plus raw accel, gyro and compass. https://github.com/sparkfun/OpenLog_Artemis/issues/47
  (done) Add support for exFAT. Requires v2.0.6 of Bill Greiman's SdFat library. https://github.com/sparkfun/OpenLog_Artemis/issues/34
  (done) Add minimum awake time: https://github.com/sparkfun/OpenLog_Artemis/issues/83
  (done) Add support for the Pulse Oximeter: https://github.com/sparkfun/OpenLog_Artemis/issues/81
  (done - but does not work) Add support for the Qwiic Button. The QB uses clock-stretching and the Artemis really doesn't enjoy that...
  (done) Increase DMP data resolution to five decimal places https://github.com/sparkfun/OpenLog_Artemis/issues/90

  (in progress) Update to Apollo3 v2.1.0 - FIRMWARE_VERSION_MAJOR = 2.
  (done) Implement printf float (OLA uses printf float in _so_ many places...): https://github.com/sparkfun/Arduino_Apollo3/issues/278
  (worked around) attachInterrupt(PIN_POWER_LOSS, powerDownOLA, FALLING); triggers an immediate interrupt - https://github.com/sparkfun/Arduino_Apollo3/issues/416
  (done) Add a setQwiicPullups function
  (done) Check if we need ap3_set_pin_to_analog when coming out of sleep
  (done) Investigate why code does not wake from deep sleep correctly
  (worked around) Correct SerialLog RX: https://github.com/sparkfun/Arduino_Apollo3/issues/401
    The work-around is to use Serial1 in place of serialLog and then to manually force UART1 to use pins 12 and 13
    We need a work-around anyway because if pins 12 or 13 have been used as analog inputs, Serial1.begin does not re-configure them for UART TX and RX
  (in progress) Reduce sleep current as much as possible. v1.2.1 achieved ~110uA. With v2.1.0 the draw is more like 260uA...

  (in progress) Update to Apollo3 v2.2.0 - FIRMWARE_VERSION_MAJOR = 2; FIRMWARE_VERSION_MINOR = 1.
  (done) Add a fix for issue #109 - check if a BME280 is connected before calling multiplexerBegin: https://github.com/sparkfun/OpenLog_Artemis/issues/109
  (done) Correct issue #104. enableSD was redundant. The microSD power always needs to be on if there is a card inserted, otherwise the card pulls
         the SPI lines low, preventing communication with the IMU:  https://github.com/sparkfun/OpenLog_Artemis/issues/104

  v2.2:
    Use Apollo3 v2.2.1 with changes by paulvha to fix Issue 117 (Thank you Paul!)
      https://github.com/sparkfun/OpenLog_Artemis/issues/117#issuecomment-1085881142
    Also includes Paul's SPI.end fix
      https://github.com/sparkfun/Arduino_Apollo3/issues/442
      In libraries/SPI/src/SPI.cpp change end() to:
        void arduino::MbedSPI::end() {
            if (dev) {
                delete dev;
                dev = NULL;
            }
        }      
    Use SdFat v2.1.2
    Compensate for missing / not-populated IMU
    Add support for yyyy/mm/dd and ISO 8601 date style (Issue 118)
    Add support for fractional time zone offsets

  v2.3 BETA:
    Change GNSS maximum rate to 25Hz as per:
      https://github.com/sparkfun/OpenLog_Artemis/issues/121
      https://forum.sparkfun.com/viewtopic.php?f=172&t=57512
    Decided not to include this in the next full release.
    I suspect it will cause major badness on (e.g.) M8 modules that cannot support 25Hz.
    
  v2.3:
    Resolve https://forum.sparkfun.com/viewtopic.php?f=171&t=58109
    
*/

//----------------------------------------------------------------------------
// WMORE defines

// WMORE version number, which is different to the Openlog Artemis version 
// number it is based on.
#define WMORE_VERSION "WMORE Coordinator v0.1" 

// WMORE defines for synchronised sampling clock
#define TIMERB_PERIOD 65535U // Free-running timer period
#define SYNC_TIMER_PERIOD 4294967295U // Free-running timer period
#define STIMER_PERIOD 4294967295U // Free-running timer period
//#define NOMINAL_PERIOD 328U // initial estimate for 100 Hz timer XT @ 32768 Hz 
//#define NOMINAL_PERIOD 1875U // initial estimate for 100 Hz timer HFRC @ 187500 Hz
#define NOMINAL_PERIOD 29700U // initial estimate for 100 Hz timer HFRC @ 3 MHz 
#define MAX_TIME_DIFF (NOMINAL_PERIOD * 1.2)  // bounds check
#define MIN_TIME_DIFF (NOMINAL_PERIOD * 0.8) // bounds check
#define MAX_SYNC (NOMINAL_PERIOD * 100) // too far out of sync?
#define PERIOD_AVG_BUFFER_SIZE 256U
//#define TIMER_CLOCK AM_HAL_CTIMER_XT_32_768KHZ
//#define TIMER_CLOCK AM_HAL_CTIMER_HFRC_187_5KHZ
#define TIMER_CLOCK AM_HAL_CTIMER_HFRC_3MHZ
#define LEDS_WAIT 0U // LEDS indicate waiting to start logging
#define LEDS_LOG 1U // LEDS indicate logging 
#define POWER_DOWN_WAIT_MS 1000U // Milliseconds to wait before powering down

//----------------------------------------------------------------------------
// WMORE timestamp structure

// Union to allow individual uint8_t access into uint32_t
union periodUnion {
  uint8_t part[4]; // individual bytes
  uint32_t full; // 32-bit word
};

// Struct for synchronisation packets
struct {
  uint8_t valid;
  uint8_t years;
  uint8_t months;
  uint8_t days;
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
  uint8_t hundredths; 
} syncPacket;

//----------------------------------------------------------------------------
// WMORE measures of time for logging

//syncUnion globalSampleTime; // (1/32768) s master clock at time of sending sync packet
//syncUnion localSampleTime; // local sample time taken from micros() 
//syncUnion intTime; // internal sample timer value at time of sync event
//syncUnion extTime; // external sync event timer value at time of sync event
periodUnion intPeriod; // internal 

uint8_t outputDataCount; // WMORE counter for binary writes to outputData
uint8_t batteryVoltage; // WMORE battery voltage 

//----------------------------------------------------------------------------
// WMORE synchronisation constants and variables, and ISRs

const int sampleTimer = 2;
//const int syncTimer = 3;
volatile uint32_t period = NOMINAL_PERIOD; // Initial estimate for timer period
volatile uint32_t extTimerValue;
volatile uint32_t intTimerValue;
volatile uint32_t timerValue; 
volatile uint32_t extTimerValueLast = 0;
volatile int32_t timerAdj = 0;
//volatile int adjust;
volatile bool triggerPinFlag = false;
volatile bool timerIntFlag = false;
volatile uint8_t periodAvgPtr = 0; // pointer into circular buffer
volatile uint32_t timeDifference;
//volatile uint32_t lastSync = 0;
//volatile uint32_t thisSync = 0;
uint32_t periodAvgBuffer[PERIOD_AVG_BUFFER_SIZE] = 
                           {NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,
                            NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD,NOMINAL_PERIOD}; // circular buffer

//----------------------------------------------------------------------------

volatile uint32_t periodSum = period * PERIOD_AVG_BUFFER_SIZE; // initialise running sum
#define DUMP( varname ) {Serial.printf("%s: %d\r\n", #varname, varname); if (settings.useTxRxPinsForTerminal == true) Serial1.printf("%s: %d\r\n", #varname, varname);}
#define SerialPrintf1( var ) {Serial.printf( var ); if (settings.useTxRxPinsForTerminal == true) Serial1.printf( var );}
#define SerialPrintf2( var1, var2 ) {Serial.printf( var1, var2 ); if (settings.useTxRxPinsForTerminal == true) Serial1.printf( var1, var2 );}
#define SerialPrintf3( var1, var2, var3 ) {Serial.printf( var1, var2, var3 ); if (settings.useTxRxPinsForTerminal == true) Serial1.printf( var1, var2, var3 );}
#define SerialPrintf4( var1, var2, var3, var4 ) {Serial.printf( var1, var2, var3, var4 ); if (settings.useTxRxPinsForTerminal == true) Serial1.printf( var1, var2, var3, var4 );}
#define SerialPrintf5( var1, var2, var3, var4, var5 ) {Serial.printf( var1, var2, var3, var4, var5 ); if (settings.useTxRxPinsForTerminal == true) Serial1.printf( var1, var2, var3, var4, var5 );}

const int FIRMWARE_VERSION_MAJOR = 2;
const int FIRMWARE_VERSION_MINOR = 3;

//Define the OLA board identifier:
//  This is an int which is unique to this variant of the OLA and which allows us
//  to make sure that the settings in EEPROM are correct for this version of the OLA
//  (sizeOfSettings is not necessarily unique and we want to avoid problems when swapping from one variant to another)
//  It is the sum of:
//    the variant * 0x100 (OLA = 1; GNSS_LOGGER = 2; GEOPHONE_LOGGER = 3)
//    the major firmware version * 0x10
//    the minor firmware version
#define OLA_IDENTIFIER 0x123 // Stored as 291 decimal in OLA_settings.txt

#include "settings.h"

#define HARDWARE_VERSION_MAJOR 1
#define HARDWARE_VERSION_MINOR 0

#if(HARDWARE_VERSION_MAJOR == 0 && HARDWARE_VERSION_MINOR == 4)
const byte PIN_MICROSD_CHIP_SELECT = 10;
const byte PIN_IMU_POWER = 22;
#elif(HARDWARE_VERSION_MAJOR == 1 && HARDWARE_VERSION_MINOR == 0)
const byte PIN_MICROSD_CHIP_SELECT = 23;
const byte PIN_IMU_POWER = 27;
const byte PIN_PWR_LED = 29;
const byte PIN_VREG_ENABLE = 25;
const byte PIN_VIN_MONITOR = 34; // VIN/3 (1M/2M - will require a correction factor)
const byte PIN_NC = 0;
#endif

const byte PIN_POWER_LOSS = 3;
//const byte PIN_LOGIC_DEBUG = 11; // Useful for debugging issues like the slippery mux bug
const byte PIN_MICROSD_POWER = 15;
const byte PIN_QWIIC_POWER = 18;
const byte PIN_STAT_LED = 19;
const byte PIN_IMU_INT = 37;
const byte PIN_IMU_CHIP_SELECT = 44;
const byte PIN_STOP_LOGGING = 32;
const byte BREAKOUT_PIN_32 = 32;
const byte BREAKOUT_PIN_TX = 12;
const byte BREAKOUT_PIN_RX = 13;
const byte BREAKOUT_PIN_11 = 11;
const byte PIN_TRIGGER = 11;
const byte PIN_QWIIC_SCL = 8;
const byte PIN_QWIIC_SDA = 9;

const byte PIN_SPI_SCK = 5;
const byte PIN_SPI_CIPO = 6;
const byte PIN_SPI_COPI = 7;

const byte SD_RECORD_LENGTH = 40; // WMORE Record length for binary file

// Include this many extra bytes when starting a mux - to try and avoid the slippery mux bug
// This should be 0 but 3 or 7 seem to work better depending on which way the wind is blowing.
const byte EXTRA_MUX_STARTUP_BYTES = 3;

enum returnStatus {
  STATUS_GETBYTE_TIMEOUT = 255,
  STATUS_GETNUMBER_TIMEOUT = -123455555,
  STATUS_PRESSED_X,
};

//Setup Qwiic Port
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <Wire.h>
TwoWire qwiic(PIN_QWIIC_SDA,PIN_QWIIC_SCL); //Will use pads 8/9
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//EEPROM for storing settings
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <EEPROM.h>
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//microSD Interface
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include <SPI.h>

#include <SdFat.h> //SdFat by Bill Greiman: http://librarymanager/All#SdFat_exFAT

#define SD_FAT_TYPE 3 // SD_FAT_TYPE = 0 for SdFat/File, 1 for FAT16/FAT32, 2 for exFAT, 3 for FAT16/FAT32 and exFAT.
#define SD_CONFIG SdSpiConfig(PIN_MICROSD_CHIP_SELECT, SHARED_SPI, SD_SCK_MHZ(24)) // 24MHz

#if SD_FAT_TYPE == 1
SdFat32 sd;
File32 sensorDataFile; //File that all sensor data is written to
File32 serialDataFile; //File that all incoming serial data is written to
#elif SD_FAT_TYPE == 2
SdExFat sd;
ExFile sensorDataFile; //File that all sensor data is written to
ExFile serialDataFile; //File that all incoming serial data is written to
#elif SD_FAT_TYPE == 3
SdFs sd;
FsFile sensorDataFile; //File that all sensor data is written to
FsFile serialDataFile; //File that all incoming serial data is written to
#else // SD_FAT_TYPE == 0
SdFat sd;
File sensorDataFile; //File that all sensor data is written to
File serialDataFile; //File that all incoming serial data is written to
#endif  // SD_FAT_TYPE

//#define PRINT_LAST_WRITE_TIME // Uncomment this line to enable the 'measure the time between writes' diagnostic

char sensorDataFileName[30] = ""; //We keep a record of this file name so that we can re-open it upon wakeup from sleep
char serialDataFileName[30] = ""; //We keep a record of this file name so that we can re-open it upon wakeup from sleep
const int sdPowerDownDelay = 100; //Delay for this many ms before turning off the SD card power
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Add RTC interface for Artemis
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include "RTC.h" //Include RTC library included with the Aruino_Apollo3 core
Apollo3RTC myRTC; //Create instance of RTC class
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Create UART instance for OpenLog style serial logging
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//UART SerialLog(BREAKOUT_PIN_TX, BREAKOUT_PIN_RX);  // Declares a Uart object called SerialLog with TX on pin 12 and RX on pin 13

//uint64_t lastSeriaLogSyncTime = 0;
uint64_t lastAwakeTimeMillis;
const int MAX_IDLE_TIME_MSEC = 500;
//bool newSerialData = false;
//char incomingBuffer[256 * 2]; //This size of this buffer is sensitive. Do not change without analysis using OpenLog_Serial.
//int incomingBufferSpot = 0;
//int charsReceived = 0; //Used for verifying/debugging serial reception
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Add ICM IMU interface
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#include "ICM_20948.h"  // Click here to get the library: http://librarymanager/All#SparkFun_ICM_20948_IMU
ICM_20948_SPI myICM;
icm_20948_DMP_data_t dmpData; // Global storage for the DMP data - extracted from the FIFO
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Header files for all compatible Qwiic sensors
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "SparkFun_I2C_Mux_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_I2C_Mux
#include "SparkFunCCS811.h" //Click here to get the library: http://librarymanager/All#SparkFun_CCS811
#include "SparkFun_VL53L1X.h" //Click here to get the library: http://librarymanager/All#SparkFun_VL53L1X
#include "SparkFunBME280.h" //Click here to get the library: http://librarymanager/All#SparkFun_BME280
#include "SparkFun_LPS25HB_Arduino_Library.h"  //Click here to get the library: http://librarymanager/All#SparkFun_LPS25HB
#include "SparkFun_VEML6075_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_VEML6075
#include "SparkFun_PHT_MS8607_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_PHT_MS8607
#include "SparkFun_MCP9600.h" //Click here to get the library: http://librarymanager/All#SparkFun_MCP9600
#include "SparkFun_SGP30_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_SGP30
#include "SparkFun_VCNL4040_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_VCNL4040
#include "SparkFun_MS5637_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_MS5637
#include "SparkFun_TMP117.h" //Click here to get the library: http://librarymanager/All#SparkFun_TMP117
#include "SparkFun_u-blox_GNSS_Arduino_Library.h" //http://librarymanager/All#SparkFun_u-blox_GNSS
#include "SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_NAU7802
#include "SparkFun_SCD30_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_SCD30
#include "SparkFun_Qwiic_Humidity_AHT20.h" //Click here to get the library: http://librarymanager/All#Qwiic_Humidity_AHT20 by SparkFun
#include "SparkFun_SHTC3.h" // Click here to get the library: http://librarymanager/All#SparkFun_SHTC3
#include "SparkFun_ADS122C04_ADC_Arduino_Library.h" // Click here to get the library: http://librarymanager/All#SparkFun_ADS122C04
#include "SparkFun_MicroPressure.h" // Click here to get the library: http://librarymanager/All#SparkFun_MicroPressure
#include "SparkFun_Particle_Sensor_SN-GCJA5_Arduino_Library.h" // Click here to get the library: http://librarymanager/All#SparkFun_Particle_Sensor_SN-GCJA5
#include "SparkFun_SGP40_Arduino_Library.h" // Click here to get the library: http://librarymanager/All#SparkFun_SGP40
#include "SparkFun_SDP3x_Arduino_Library.h" // Click here to get the library: http://librarymanager/All#SparkFun_SDP3x
// WMORE MS5837 disabled due to compile errors
//#include "MS5837.h" // Click here to download the library: https://github.com/sparkfunX/BlueRobotics_MS5837_Library
#include "SparkFun_Qwiic_Button.h" // Click here to get the library: http://librarymanager/All#SparkFun_Qwiic_Button_Switch
#include "SparkFun_Bio_Sensor_Hub_Library.h" // Click here to get the library: http://librarymanager/All#SparkFun_Bio_Sensor

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//Global variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
uint64_t measurementStartTime; //Used to calc the actual update rate. Max is ~80,000,000ms in a 24 hour period.
uint64_t lastSDFileNameChangeTime; //Used to calculate the interval since the last SD filename change
unsigned long measurementCount = 0; //Used to calc the actual update rate.
unsigned long measurementTotal = 0; //The total number of recorded measurements. (Doesn't get reset when the menu is opened)
char outputData[512 * 2]; //Factor of 512 for easier recording to SD in 512 chunks
//unsigned long lastReadTime = 0; //Used to delay until user wants to record a new reading
unsigned long lastDataLogSyncTime = 0; //Used to record to SD every half second
unsigned int totalCharactersPrinted = 0; //Limit output rate based on baud rate and number of characters to print
bool takeReading = true; //Goes true when enough time has passed between readings or we've woken from sleep
bool sleepAfterRead = false; //Used to keep the code awake for at least minimumAwakeTimeMillis
const uint64_t maxUsBeforeSleep = 2000000ULL; //Number of us between readings before sleep is activated.
const byte menuTimeout = 15; //Menus will exit/timeout after this number of seconds
volatile static bool stopLoggingSeen = false; //Flag to indicate if we should stop logging
uint64_t qwiicPowerOnTime = 0; //Used to delay after Qwiic power on to allow sensors to power on, then answer autodetect
unsigned long qwiicPowerOnDelayMillis; //Wait for this many milliseconds after turning on the Qwiic power before attempting to communicate with Qwiic devices
int lowBatteryReadings = 0; // Count how many times the battery voltage has read low
const int lowBatteryReadingsLimit = 10; // Don't declare the battery voltage low until we have had this many consecutive low readings (to reject sampling noise)
volatile static bool triggerEdgeSeen = false; //Flag to indicate if a trigger interrupt has been seen
char serialTimestamp[40]; //Buffer to store serial timestamp, if needed
volatile static bool powerLossSeen = false; //Flag to indicate if a power loss event has been seen

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

uint8_t getByteChoice(int numberOfSeconds, bool updateDZSERIAL = false); // Header

// The Serial port for the Zmodem connection
// must not be the same as DSERIAL unless all
// debugging output to DSERIAL is removed
Stream *ZSERIAL;

// Serial output for debugging info for Zmodem
Stream *DSERIAL;

//----------------------------------------------------------------------------
// WMORE
// Sample timer ISR

#ifdef __cplusplus
  extern "C" {
#endif
// C++ compilations must declare the ISR as a "C" routine or else its name will get mangled
// and the linker will not use this routine to replace the default ISR
void timerISR();

#ifdef __cplusplus
  }
#endif

extern "C" void timerISR()
{
  am_hal_ctimer_period_set(sampleTimer, AM_HAL_CTIMER_TIMERA, period + timerAdj, 1); // adjust period once
  am_hal_ctimer_int_clear(AM_HAL_CTIMER_INT_TIMERA2); // clear the timer interrupt
  timerIntFlag = true; // trigger IMU sampling
  timerAdj = 0; // DON'T FORGET TO DO THIS, EITHER IN ISR OR MAIN LOOP!!!
}

//----------------------------------------------------------------------------
// WMORE
// Synchronisation input ISR

#ifdef __cplusplus
  extern "C" {
#endif
// C++ compilations must declare the ISR as a "C" routine or else its name will get mangled
// and the linker will not use this routine to replace the default ISR
void triggerPinISR();

#ifdef __cplusplus
  }
#endif

extern "C" void triggerPinISR() {

  // Process external sync interrupt and calculate adjustment
  // Get interrupt flags
  uint64_t gpio_int_mask = 0x00;
  
  am_hal_gpio_interrupt_status_get(true, &gpio_int_mask);
  am_hal_gpio_interrupt_clear(AM_HAL_GPIO_BIT(gpio_int_mask)); // clear GPIO interrupt
  triggerPinFlag = true;
  
// Disabled for RTC coordinator
//  // read the current internal timer (sampling timer) value
//  intTimerValue = am_hal_ctimer_read(sampleTimer, AM_HAL_CTIMER_TIMERA);
//  // read the current external event timer (sync timer) value
//  //extTimerValue = am_hal_ctimer_read(sampleTimer, AM_HAL_CTIMER_TIMERB);
//  // extTimerValue = am_hal_ctimer_read(syncTimer, AM_HAL_CTIMER_BOTH);
//  extTimerValue = am_hal_stimer_counter_get(); // Now using STIMER instead of timer 3 
//  am_hal_gpio_interrupt_clear(AM_HAL_GPIO_BIT(gpio_int_mask)); // clear GPIO interrupt
//  adjustTime(); // Adjust the sampling timer
//  // store previous timer value
//  extTimerValueLast = extTimerValue;   
//  //myRTC.getTime(); // Get the time from the RTC
}

//----------------------------------------------------------------------------
// WMORE 
// Engage burst mode
// Based on Ambiq burst mode example

void burstMode(void) {
  
  am_hal_burst_avail_e          eBurstModeAvailable;
  am_hal_burst_mode_e           eBurstMode;
  
  am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0);
  //am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
  //am_hal_cachectrl_enable();
  //am_bsp_low_power_init();
  am_hal_burst_mode_initialize(&eBurstModeAvailable);
  am_hal_burst_mode_enable(&eBurstMode);
}
//----------------------------------------------------------------------------
// WMORE 
// Set clock state

void clockState(void) {

  burstMode(); // Engage burst mode - disabled for Coordinator

  //am_hal_stimer_config(AM_HAL_STIMER_XTAL_32KHZ);
  //
  // Enable the XT for the RTC.
  //
  am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_XTAL_START, 0);

  //
  // Select XT for RTC clock source
  //
  am_hal_rtc_osc_select(AM_HAL_RTC_OSC_XT);

  //
  // Enable the RTC.
  //
  am_hal_rtc_osc_enable();  
  
  delay(1); // wait for clock to settle
  
  // Enable HFADJ 
  // TODO: confirm this is working
  am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_HFADJ_ENABLE, 0);  
}

//----------------------------------------------------------------------------
// WMORE 
// Configure sample timer

void setupSampleTimer(int timerNum, uint32_t periodTicks)
{
  //  Refer to Ambiq Micro Apollo3 Blue MCU datasheet section 13.2.2
  //  and am_hal_ctimer.c line 710 of 2210.
  //
   am_hal_ctimer_config_single(timerNum, AM_HAL_CTIMER_TIMERA,
                              TIMER_CLOCK |
                              AM_HAL_CTIMER_FN_REPEAT |
                              AM_HAL_CTIMER_INT_ENABLE);
  //
  //  Repeated Count: Periodic 1-clock-cycle wide pulses with optional interrupts.
  //  The last parameter to am_hal_ctimer_period_set() has no effect in this mode.
  //
  am_hal_ctimer_period_set(timerNum, AM_HAL_CTIMER_TIMERA, periodTicks, 1);
}
//----------------------------------------------------------------------------
// WMORE
// Configure sync timer (not used, uses STIMER instead)

void setupSyncTimer(int timerNum, uint32_t periodTicks)
{
  //  Refer to Ambiq Micro Apollo3 Blue MCU datasheet section 13.2.2
  //  and am_hal_ctimer.c line 710 of 2210.
  //
  am_hal_ctimer_config_single(timerNum, AM_HAL_CTIMER_BOTH,
                              TIMER_CLOCK |
                              AM_HAL_CTIMER_FN_REPEAT);

  am_hal_ctimer_period_set(timerNum, AM_HAL_CTIMER_BOTH, periodTicks, 1);
}

//----------------------------------------------------------------------------
// WMORE
// Adjust timer A period based on valid period information
// from the external sync interrupt.

void adjustTime(void) {
  if (extTimerValue >= extTimerValueLast) { 
    // calculate period between most recent sync events
    timeDifference = extTimerValue - extTimerValueLast; 
  } else { // Sync timer rollover case
    timeDifference = STIMER_PERIOD - extTimerValueLast + extTimerValue;
  }  
  // update the external sync period moving average
  // bounds check
  if ((timeDifference <= MAX_TIME_DIFF) && (timeDifference >= MIN_TIME_DIFF)) { 
    // Add the newest sample and subtract the oldest from the running sum
    periodSum = periodSum + timeDifference - periodAvgBuffer[periodAvgPtr]; 
    // Overwrite the oldest sample with the newest sample
    periodAvgBuffer[periodAvgPtr] = timeDifference; 
    // Calculate the average
    period = periodSum / PERIOD_AVG_BUFFER_SIZE;
    // Increment the buffer pointer, modulo the buffer size
    periodAvgPtr++ % PERIOD_AVG_BUFFER_SIZE;
  }
  // Calculate a period adjustment value to be applied once
  // When did the sync event happen relative to the local timer  
  if (intTimerValue < (period / 2)) { 
    timerAdj = (intTimerValue / 1); // try proportional to distance to desired timeout (/4, /2, /1 tried) 
  } else {
    timerAdj = -1 * (((period - intTimerValue) / 1)); // try proportional to distance to desired timeout (/4, /2, /1 tried)
  } 
}

//----------------------------------------------------------------------------
// WMORE 
// Set up synchronisation resources

void setupSync(void) {
  
  pinMode(PIN_TRIGGER, INPUT_PULLUP);
  delay(1);
  attachInterrupt(PIN_TRIGGER, triggerPinISR, FALLING); // Enable the interrupt
  am_hal_gpio_pincfg_t intPinConfig = g_AM_HAL_GPIO_INPUT_PULLUP;
  intPinConfig.eIntDir = AM_HAL_GPIO_PIN_INTDIR_HI2LO;
  pin_config(PinName(PIN_TRIGGER), intPinConfig); // Make sure the pull-up does actually stay enabled
  triggerPinFlag = false; // Make sure the flag is clear

// Sample timer disabled for RTC coordinator
//  // Timer A: sets sampling interval
//  setupSampleTimer(sampleTimer, period); // timerNum, period, padNum
//  am_hal_ctimer_start(sampleTimer, AM_HAL_CTIMER_TIMERA);
//  // Set up timer A interrupts
//  NVIC_EnableIRQ(CTIMER_IRQn);
//  am_hal_ctimer_int_clear(AM_HAL_CTIMER_INT_TIMERA2);
//  am_hal_ctimer_int_enable(AM_HAL_CTIMER_INT_TIMERA2);  
//  am_hal_ctimer_int_register(AM_HAL_CTIMER_INT_TIMERA2, timerISR);
  am_hal_interrupt_master_enable();
}

//----------------------------------------------------------------------------
// WMORE
// Modified and simplified setup()

void setup() {

  // Ensure clocks are in a known state
  clockState();
  // Set up synchronisation inputs, timers, and ISRs
  // Only uses triggerPinISR() for RTC Coordinator
  setupSync();
  
  //If 3.3V rail drops below 3V, system will power down and maintain RTC
  pinMode(PIN_POWER_LOSS, INPUT); // BD49K30G-TL has CMOS output and does not need a pull-up

  delay(1); // Let PIN_POWER_LOSS stabilize

  if (digitalRead(PIN_POWER_LOSS) == LOW) powerDownOLA(); //Check PIN_POWER_LOSS just in case we missed the falling edge
  //attachInterrupt(PIN_POWER_LOSS, powerDownOLA, FALLING); // We can't do this with v2.1.0 as attachInterrupt causes a spontaneous interrupt
  attachInterrupt(PIN_POWER_LOSS, powerLossISR, FALLING);
  powerLossSeen = false; // Make sure the flag is clear

  pinMode(PIN_STAT_LED, OUTPUT);
  digitalWrite(PIN_STAT_LED, HIGH); // Turn the STAT LED on while we configure everything

  SPI.begin(); //Needed if SD is disabled

  configureSerial1TxRx();

  Serial.begin(115200); //Default for initial debug messages if necessary
  Serial1.begin(460800); // Set up to transmit/receive global timestamps

  EEPROM.init();

  beginSD(); //285 - 293ms

  enableCIPOpullUp(); // Enable CIPO pull-up _after_ beginSD

  loadSettings(); //50 - 250ms

  overrideSettings(); // Hard-code some critical settings to avoid accidents

  Serial.flush(); //Complete any previous prints
  Serial.begin(settings.serialTerminalBaudRate); // TODO settings

  Serial.println(WMORE_VERSION);

  pinMode(PIN_STOP_LOGGING, INPUT_PULLUP);
  delay(1); // Let the pin stabilize
  attachInterrupt(PIN_STOP_LOGGING, stopLoggingISR, FALLING); // Enable the interrupt
  am_hal_gpio_pincfg_t intPinConfig = g_AM_HAL_GPIO_INPUT_PULLUP;
  intPinConfig.eIntDir = AM_HAL_GPIO_PIN_INTDIR_HI2LO;
  pin_config(PinName(PIN_STOP_LOGGING), intPinConfig); // Make sure the pull-up does actually stay enabled
  stopLoggingSeen = false; // Make sure the flag is clear
 
  analogReadResolution(14); //Increase from default of 10

  beginDataLogging(); //180ms WMORE not used for Coordinator
  lastSDFileNameChangeTime = rtcMillis(); // Record the time of the file name change WMORE not used for Coordinator

  //beginIMU(); //61ms WMORE not used for Coordinator

//  if (online.microSD == true) SerialPrintln(F("SD card online"));
//  else SerialPrintln(F("SD card offline"));
//
//  if (online.dataLogging == true) SerialPrintln(F("Data logging online"));
//  else SerialPrintln(F("Datalogging offline"));
//
//  if (online.IMU == true) SerialPrintln(F("IMU online"));
//  else SerialPrintln(F("IMU offline - or not present"));

  digitalWrite(PIN_STAT_LED, LOW); // Turn the blue LED off - setup finished

  waitToLog(); 
//  while (digitalRead(PIN_TRIGGER) == 1) { // Wait for first sync falling edge  
//    if (Serial.available()) {
//      menuMain(); //Present user menu if serial character received
//    }; 
//    checkBattery(); // Check for low battery
//  }
//  powerLEDOff(); // Turn off red LED during logging
}

//----------------------------------------------------------------------------
// WMORE
// Modified and simplified loop()

void loop() {

  // Sampling driven by timer
  //while (timerIntFlag != true) {}; // Wait for sampling timer

  if (triggerPinFlag == true) {
    triggerPinFlag = false;
    digitalWrite(PIN_STAT_LED, HIGH); // Turn on blue LED
    myRTC.getTime(); // Get the time from the RTC
    sendRTC(); // Send the RTC value to the ESB transmitter (Coordinator use only) 
    digitalWrite(PIN_STAT_LED, LOW); // Turn off blue LED
    if (stopLoggingSeen == true) {
      stopLoggingSeen = false; // Reset the flag
      waitToLog();
      //delay(POWER_DOWN_WAIT_MS); // Give the Nano time to transmit the stop command
      //powerDownOLA(); // Enter low power sleep; cycle power to reset
    } 
  }

  if (Serial.available()) {
    menuMain(); //Present user menu if serial character received
  }  
  checkBattery(); // Check for low battery
  
}

//----------------------------------------------------------------------------

void beginSD()
{
  pinMode(PIN_MICROSD_POWER, OUTPUT);
  pin_config(PinName(PIN_MICROSD_POWER), g_AM_HAL_GPIO_OUTPUT); // Make sure the pin does actually get re-configured
  pinMode(PIN_MICROSD_CHIP_SELECT, OUTPUT);
  pin_config(PinName(PIN_MICROSD_CHIP_SELECT), g_AM_HAL_GPIO_OUTPUT); // Make sure the pin does actually get re-configured
  digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected

  // If the microSD card is present, it needs to be powered on otherwise the IMU will fail to start
  // (The microSD card will pull the SPI pins low, preventing communication with the IMU)

  // For reasons I don't understand, we seem to have to wait for at least 1ms after SPI.begin before we call microSDPowerOn.
  // If you comment the next line, the Artemis resets at microSDPowerOn when beginSD is called from wakeFromSleep...
  // But only on one of my V10 red boards. The second one I have doesn't seem to need the delay!?
  delay(5);

  microSDPowerOn();

  //Max power up time is 250ms: https://www.kingston.com/datasheets/SDCIT-specsheet-64gb_en.pdf
  //Max current is 200mA average across 1s, peak 300mA
  for (int i = 0; i < 10; i++) //Wait
  {
    checkBattery();
    delay(1);
  }

  if (sd.begin(SD_CONFIG) == false) // Try to begin the SD card using the correct chip select
  {
    printDebug(F("SD init failed (first attempt). Trying again...\r\n"));
    for (int i = 0; i < 250; i++) //Give SD more time to power up, then try again
    {
      checkBattery();
      delay(1);
    }
    if (sd.begin(SD_CONFIG) == false) // Try to begin the SD card using the correct chip select
    {
      SerialPrintln(F("SD init failed (second attempt). Is card present? Formatted?"));
      SerialPrintln(F("Please ensure the SD card is formatted correctly using https://www.sdcard.org/downloads/formatter/"));
      digitalWrite(PIN_MICROSD_CHIP_SELECT, HIGH); //Be sure SD is deselected
      online.microSD = false;
      return;
    }
  }

  //Change to root directory. All new file creation will be in root.
  if (sd.chdir() == false)
  {
    SerialPrintln(F("SD change directory failed"));
    online.microSD = false;
    return;
  }

  online.microSD = true;
}

void enableCIPOpullUp() // updated for v2.1.0 of the Apollo3 core
{
  //Add 1K5 pull-up on CIPO
  am_hal_gpio_pincfg_t cipoPinCfg = g_AM_BSP_GPIO_IOM0_MISO;
  cipoPinCfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_1_5K;
  pin_config(PinName(PIN_SPI_CIPO), cipoPinCfg);
}

void disableCIPOpullUp() // updated for v2.1.0 of the Apollo3 core
{
  am_hal_gpio_pincfg_t cipoPinCfg = g_AM_BSP_GPIO_IOM0_MISO;
  pin_config(PinName(PIN_SPI_CIPO), cipoPinCfg);
}

void configureSerial1TxRx(void) // Configure pins 12 and 13 for UART1 TX and RX
{
  am_hal_gpio_pincfg_t pinConfigTx = g_AM_BSP_GPIO_COM_UART_TX;
  pinConfigTx.uFuncSel = AM_HAL_PIN_12_UART1TX;
  pin_config(PinName(BREAKOUT_PIN_TX), pinConfigTx);
  am_hal_gpio_pincfg_t pinConfigRx = g_AM_BSP_GPIO_COM_UART_RX;
  pinConfigRx.uFuncSel = AM_HAL_PIN_13_UART1RX;
  pinConfigRx.ePullup = AM_HAL_GPIO_PIN_PULLUP_WEAK; // Put a weak pull-up on the Rx pin
  pin_config(PinName(BREAKOUT_PIN_RX), pinConfigRx);
}

//----------------------------------------------------------------------------

void beginIMU()
{
  pinMode(PIN_IMU_POWER, OUTPUT);
  pin_config(PinName(PIN_IMU_POWER), g_AM_HAL_GPIO_OUTPUT); // Make sure the pin does actually get re-configured
  pinMode(PIN_IMU_CHIP_SELECT, OUTPUT);
  pin_config(PinName(PIN_IMU_CHIP_SELECT), g_AM_HAL_GPIO_OUTPUT); // Make sure the pin does actually get re-configured
  digitalWrite(PIN_IMU_CHIP_SELECT, HIGH); //Be sure IMU is deselected

  if (settings.enableIMU == true && settings.logMaxRate == false)
  {
    //Reset ICM by power cycling it
    imuPowerOff();
    for (int i = 0; i < 10; i++) //10 is fine
    {
      checkBattery();
      delay(1);
    }
    imuPowerOn();
    for (int i = 0; i < 25; i++) //Allow ICM to come online. Typical is 11ms. Max is 100ms. https://cdn.sparkfun.com/assets/7/f/e/c/d/DS-000189-ICM-20948-v1.3.pdf
    {
      checkBattery();
      delay(1);
    }

    if (settings.printDebugMessages) myICM.enableDebugging();
    myICM.begin(PIN_IMU_CHIP_SELECT, SPI, 7000000); // WMORE set IMU SPI rate to 7 MHz
    if (myICM.status != ICM_20948_Stat_Ok)
    {
      printDebug("beginIMU: first attempt at myICM.begin failed. myICM.status = " + (String)myICM.status + "\r\n");
      //Try one more time with longer wait

      //Reset ICM by power cycling it
      imuPowerOff();
      for (int i = 0; i < 10; i++) //10 is fine
      {
        checkBattery();
        delay(1);
      }
      imuPowerOn();
      for (int i = 0; i < 100; i++) //Allow ICM to come online. Typical is 11ms. Max is 100ms.
      {
        checkBattery();
        delay(1);
      }

      myICM.begin(PIN_IMU_CHIP_SELECT, SPI, 7000000); // WMORE set IMU SPI rate to 7MHz
      if (myICM.status != ICM_20948_Stat_Ok)
      {
        printDebug("beginIMU: second attempt at myICM.begin failed. myICM.status = " + (String)myICM.status + "\r\n");
        digitalWrite(PIN_IMU_CHIP_SELECT, HIGH); //Be sure IMU is deselected
        SerialPrintln(F("ICM-20948 failed to init."));
        imuPowerOff();
        online.IMU = false;
        return;
      }
    }

    //Give the IMU extra time to get its act together. This seems to fix the IMU-not-starting-up-cleanly-after-sleep problem...
    //Seems to need a full 25ms. 10ms is not enough.
    for (int i = 0; i < 25; i++) //Allow ICM to come online.
    {
      checkBattery();
      delay(1);
    }

    bool success = true;

    //Check if we are using the DMP
    if (settings.imuUseDMP == false)
    {
      //Perform a full startup (not minimal) for non-DMP mode
      ICM_20948_Status_e retval = myICM.startupDefault(false);
      if (retval != ICM_20948_Stat_Ok)
      {
        SerialPrintln(F("Error: Could not startup the IMU in non-DMP mode!"));
        success = false;
      }

      ICM_20948_smplrt_t mySmplrt;
      mySmplrt.a = 0;  // maximum accel sample rate: see Table 19 in datasheet DS-000189-ICM-20948-v1.5.pdf
      mySmplrt.g = 0;  // maximum gyro sample rate: see Table 17 in datasheet DS-000189-ICM-20948-v1.5.pdf 
      myICM.setSampleRate( ICM_20948_Internal_Acc, mySmplrt );
      myICM.setSampleRate( ICM_20948_Internal_Gyr, mySmplrt );
  
      //Update the full scale and DLPF settings
      retval = myICM.enableDLPF(ICM_20948_Internal_Acc, settings.imuAccDLPF);
      if (retval != ICM_20948_Stat_Ok)
      {
        SerialPrintln(F("Error: Could not configure the IMU Accelerometer DLPF!"));
        success = false;
      }
      retval = myICM.enableDLPF(ICM_20948_Internal_Gyr, settings.imuGyroDLPF);
      if (retval != ICM_20948_Stat_Ok)
      {
        SerialPrintln(F("Error: Could not configure the IMU Gyro DLPF!"));
        success = false;
      }
      ICM_20948_dlpcfg_t dlpcfg;
      dlpcfg.a = settings.imuAccDLPFBW;
      dlpcfg.g = settings.imuGyroDLPFBW;
      retval = myICM.setDLPFcfg((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), dlpcfg);
      if (retval != ICM_20948_Stat_Ok)
      {
        SerialPrintln(F("Error: Could not configure the IMU DLPF BW!"));
        success = false;
      }
      ICM_20948_fss_t FSS;
      //Serial.println(settings.imuGyroFSS);
      FSS.a = settings.imuAccFSS;
      FSS.g = settings.imuGyroFSS;
      retval = myICM.setFullScale((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), FSS);
      if (retval != ICM_20948_Stat_Ok)
      {
        SerialPrintln(F("Error: Could not configure the IMU Full Scale!"));
        success = false;
      }
    }
    
    if (success)
    {
      online.IMU = true;
      delay(50); // Give the IMU time to get its first measurement ready
    }
    else
    {
      //Power down IMU
      imuPowerOff();
      online.IMU = false;
    }
  }
  else
  {
    //Power down IMU
    imuPowerOff();
    online.IMU = false;
  }
}

//----------------------------------------------------------------------------

void beginDataLogging()
{
  if (online.microSD == true && settings.logData == true) // TODO settings
  {
    //If we don't have a file yet, create one. Otherwise, re-open the last used file
    if (strlen(sensorDataFileName) == 0)
      //strcpy(sensorDataFileName, findNextAvailableLog(settings.nextDataLogNumber, "dataLog"));
      strcpy(sensorDataFileName, generateFileName()); // Create file name   
    // O_CREAT - create the file if it does not exist
    // O_APPEND - seek to the end of the file prior to each write
    // O_WRITE - open for write
    if (sensorDataFile.open(sensorDataFileName, O_CREAT | O_APPEND | O_WRITE) == false)
    {
      SerialPrintln(F("Failed to create sensor data file"));
      online.dataLogging = false;
      return;
    }

    updateDataFileCreate(&sensorDataFile); // Update the file create time & date
    sensorDataFile.sync();

    online.dataLogging = true;
  }
  else
    online.dataLogging = false;
}

#if SD_FAT_TYPE == 1
void updateDataFileCreate(File32 *dataFile)
#elif SD_FAT_TYPE == 2
void updateDataFileCreate(ExFile *dataFile)
#elif SD_FAT_TYPE == 3
void updateDataFileCreate(FsFile *dataFile)
#else // SD_FAT_TYPE == 0
void updateDataFileCreate(File *dataFile)
#endif  // SD_FAT_TYPE
{
  myRTC.getTime(); //Get the RTC time so we can use it to update the create time
  //Update the file create time
  dataFile->timestamp(T_CREATE, (myRTC.year + 2000), myRTC.month, myRTC.dayOfMonth, myRTC.hour, myRTC.minute, myRTC.seconds);
}

#if SD_FAT_TYPE == 1
void updateDataFileAccess(File32 *dataFile)
#elif SD_FAT_TYPE == 2
void updateDataFileAccess(ExFile *dataFile)
#elif SD_FAT_TYPE == 3
void updateDataFileAccess(FsFile *dataFile)
#else // SD_FAT_TYPE == 0
void updateDataFileAccess(File *dataFile)
#endif  // SD_FAT_TYPE
{
  myRTC.getTime(); //Get the RTC time so we can use it to update the last modified time
  //Update the file access time
  dataFile->timestamp(T_ACCESS, (myRTC.year + 2000), myRTC.month, myRTC.dayOfMonth, myRTC.hour, myRTC.minute, myRTC.seconds);
  //Update the file write time
  dataFile->timestamp(T_WRITE, (myRTC.year + 2000), myRTC.month, myRTC.dayOfMonth, myRTC.hour, myRTC.minute, myRTC.seconds);
}

//Called once number of milliseconds has passed
extern "C" void am_stimer_cmpr6_isr(void)
{
  uint32_t ui32Status = am_hal_stimer_int_status_get(false);
  if (ui32Status & AM_HAL_STIMER_INT_COMPAREG)
  {
    am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREG);
  }
}

//----------------------------------------------------------------------------

//Power Loss ISR
void powerLossISR(void)
{
  powerLossSeen = true;
}

//----------------------------------------------------------------------------

// Stop Logging ISR
void stopLoggingISR(void)
{
  stopLoggingSeen = true;
}

//----------------------------------------------------------------------------

void SerialFlush(void)
{
  Serial.flush();
  if (settings.useTxRxPinsForTerminal == true)
  {
    Serial1.flush();
  }
}

//----------------------------------------------------------------------------
// WMORE
// Clear received serial characters. 
void serialClearBuffer(uint8_t port)
{
  uint8_t data;
  
  switch(port)
  {
    case 0:
      while(Serial.available() > 0) 
      {
        data = Serial.read();         
      }
      break;
    case 1:
      while(Serial1.available() > 0) 
      {
        data = Serial1.read();         
      }
      break; 
    default:
      break;       
  }
}

//----------------------------------------------------------------------------
// WMORE
// Binary SD file format to reduce file size and increase write speed.
void writeSDBin(void) {

  if (online.microSD)
  {
    // Write binary data
    uint32_t recordLength = sensorDataFile.write(outputData, SD_RECORD_LENGTH); // WMORE TODO add error checking and resolve hard-coded length        
      
  //Force sync every 500ms
//  if (rtcMillis() - lastDataLogSyncTime > 500)
//  {
//    lastDataLogSyncTime = rtcMillis();
//    sensorDataFile.sync();
//    if (settings.frequentFileAccessTimestamps == true)
//      updateDataFileAccess(&sensorDataFile); // Update the file access time & date
//  }

  //Check if it is time to open a new log file
  uint64_t secsSinceLastFileNameChange = rtcMillis() - lastSDFileNameChangeTime; // Calculate how long we have been logging for
  secsSinceLastFileNameChange /= 1000ULL; // Convert to secs
  if ((settings.openNewLogFilesAfter > 0) && (((unsigned long)secsSinceLastFileNameChange) >= settings.openNewLogFilesAfter))
  {
    //Close existing files
    if (online.dataLogging == true)
    {
      sensorDataFile.sync();
      updateDataFileAccess(&sensorDataFile); // Update the file access time & date
      sensorDataFile.close();
      //strcpy(sensorDataFileName, findNextAvailableLog(settings.nextDataLogNumber, "dataLog"));
      strcpy(sensorDataFileName, generateFileName()); // Create file name   
      beginDataLogging(); //180ms
    }
    lastSDFileNameChangeTime = rtcMillis(); // Record the time of the file name change
  }

  //}

//    if ((settings.useGPIO32ForStopLogging == true) && (stopLoggingSeen == true)) // Has the user pressed the stop logging button?
//    {
//      stopLogging();
//    }  
  }
}  
//----------------------------------------------------------------------------

// WMORE
// Overrides EEPROM settings where conflicts with new functionality may occur.
// The settings struct is defined in settings.h
void overrideSettings(void) {

//settings.sizeOfSettings = 168;
//settings.sizeOfSettings = sizeof(settings);
//settings.olaIdentifier = 291; // Depends on Sparkfun OLA base version
//settings.nextSerialLogNumber = 1; // Not used
//settings.nextDataLogNumber = 13; // Not used
  settings.usBetweenReadings = 10000;
  settings.logMaxRate = 0;
  settings.enableRTC = 1; // Alternative implementation
  settings.enableIMU = 0; 
  settings.enableTerminalOutput = 0;
  settings.logDate = 1; // Alternative implementation
  settings.logTime = 1; // Alternative implementation
  settings.logData = 1;
  settings.logSerial = 0;
  settings.logIMUAccel = 0; // Alternative implementation
  settings.logIMUGyro = 0; // Alternative implementation
  settings.logIMUMag = 0; // Alternative implementation
  settings.logIMUTemp = 0; // Alternative implementation
  settings.logRTC = 1; // Alternative implementation
  settings.logHertz = 0; // Alternative implementation
  settings.correctForDST = 0;
  settings.dateStyle = 1; // dd/mm/yy
  settings.hour24Style = 1; // 24 hour
//settings.serialTerminalBaudRate = 115200; // User set
  settings.serialLogBaudRate = 9600; // Disabled 
  settings.showHelperText = 0; // Disabled
  settings.logA11 = 0; // Disabled
  settings.logA12 = 0; // Disabled
  settings.logA13 = 0; // Disabled
  settings.logA32 = 0; // Disabled
  settings.logAnalogVoltages = 0; // Disabled
  settings.localUTCOffset = 0.00;
  settings.printDebugMessages = 0; // Disabled
  settings.powerDownQwiicBusBetweenReads = 0; // Disabled
  settings.qwiicBusMaxSpeed = 100000; // Disabled
  settings.qwiicBusPowerUpDelayMs = 250; // Disabled
  settings.printMeasurementCount = 0; // Disabled
  settings.enablePwrLedDuringSleep = 0; // Disabled for Coordinator
  settings.logVIN = 0; // Disabled
//settings.openNewLogFilesAfter = 0; // User set
  settings.vinCorrectionFactor = 1.47; // 
  settings.useGPIO32ForStopLogging = 1;
  settings.qwiicBusPullUps = 1; // Disabled
  settings.outputSerial = 0; // Disabled
//settings.zmodemStartDelay = 20; // User set
//settings.enableLowBatteryDetection = 0; // User set
//settings.lowBatteryThreshold = 3.40; // User set
  settings.frequentFileAccessTimestamps = 0; // Disabled
  settings.useGPIO11ForTrigger = 0; // Disabled
  settings.fallingEdgeTrigger = 1; // Disabled
//settings.imuAccDLPF = 1; // User set
//settings.imuGyroDLPF = 1; // User set
//settings.imuAccFSS = 1; // User set
//settings.imuAccDLPFBW = 4; // User set
//settings.imuGyroFSS = 2; // User set
//settings.imuGyroDLPFBW = 4; // User set
  settings.logMicroseconds = 0; // Disabled
  settings.useTxRxPinsForTerminal = 0; // Disabled
  settings.timestampSerial = 0; // Disabled
  settings.timeStampToken = 10; // Disabled
  settings.useGPIO11ForFastSlowLogging = 0; // Disabled
  settings.slowLoggingWhenPin11Is = 0; // Disabled
  settings.useRTCForFastSlowLogging = 0; // Disabled
  settings.slowLoggingIntervalSeconds = 300; // Disabled
  settings.slowLoggingStartMOD = 1260; // Disabled
  settings.slowLoggingStopMOD = 420; // Disabled
  settings.resetOnZeroDeviceCount = 0; // Disabled
  settings.imuUseDMP = 0; // Disabled
  settings.imuLogDMPQuat6 = 0; // Disabled
  settings.imuLogDMPQuat9 = 0; // Disabled
  settings.imuLogDMPAccel = 0; // Disabled
  settings.imuLogDMPGyro = 0; // Disabled
  settings.imuLogDMPCpass = 0; // Disabled
  settings.minimumAwakeTimeMillis = 0; // Disabled
  settings.identifyBioSensorHubs = 0; // Disabled
  settings.serialTxRxDuringSleep = 0; // Disabled
  settings.printGNSSDebugMessages = 0; // Disabled
//settings.serialNumber = 0; // User set
}

//----------------------------------------------------------------------------

// WMORE
// Sends RTC value via Serial1. If this unit is a Coordinator, this is broadcast
// to all Sensors.

void sendRTC(void) {
  
  uint8_t rtcBuf[7];
  uint8_t i;

  // Fill buffer with current RTC value.
  rtcBuf[0] = (uint8_t)myRTC.year;
  rtcBuf[1] = (uint8_t)myRTC.month;
  rtcBuf[2] = (uint8_t)myRTC.dayOfMonth;
  rtcBuf[3] = (uint8_t)myRTC.hour;
  rtcBuf[4] = (uint8_t)myRTC.minute;
  rtcBuf[5] = (uint8_t)myRTC.seconds;
  rtcBuf[6] = (uint8_t)myRTC.hundredths;

  // Send RTC value to ESB transmitter.
  Serial1.write(rtcBuf,7); // Fast - 7 bytes in 150 us @ 460800 bps
  
}

//----------------------------------------------------------------------------
// WMORE
// Waits for trigger to start logging

void waitToLog(void) {
  
  powerLEDOn(); // Turn on the red LED
  while (digitalRead(PIN_TRIGGER) == 1) { // Wait for sync falling edge  
    if (Serial.available()) {
      menuMain(); //Present user menu if serial character received
    }; 
    checkBattery(); // Check for low battery and shutdown if low
  }
  powerLEDOff(); // Turn off the red LED  
    
}
