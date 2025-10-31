typedef struct {
  uint32_t unix;
  uint8_t hundredths;
} UnixTimeWithHunds;

typedef enum
{
  DEVICE_MULTIPLEXER = 0,
  DEVICE_TOTAL_DEVICES, //Marks the end, used to iterate loops
  DEVICE_UNKNOWN_DEVICE,
} deviceType_e;

struct node
{
  deviceType_e deviceType;

  uint8_t address = 0;
  uint8_t portNumber = 0;
  uint8_t muxAddress = 0;
  bool online = false; //Goes true once successfully begin()'d

  void *classPtr; //Pointer to this devices' class instantiation
  void *configPtr; //The struct containing this devices logging options
  node *next;
};

node *head = NULL;
node *tail = NULL;

typedef void (*FunctionPointer)(void*); //Used for pointing to device config menus

//Begin specific sensor config structs
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

const unsigned long worstCaseQwiicPowerOnDelay = 1000; // Remember to update this if required when adding a new sensor (currently defined by the u-blox). (This is OK for the SCD30. beginQwiicDevices will extend the delay.)
const unsigned long minimumQwiicPowerOnDelay = 10; //The minimum number of milliseconds between turning on the Qwiic power and attempting to communicate with Qwiic devices

struct struct_multiplexer {
  //Ignore certain ports at detection step?
  unsigned long powerOnDelayMillis = minimumQwiicPowerOnDelay; // Wait for at least this many millis before communicating with this device. Increase if required!
};

//This is all the settings that can be set on OpenLog. It's recorded to NVM and the config file.
struct struct_settings {
  int sizeOfSettings = 0; //sizeOfSettings **must** be the first entry and must be int
  int olaIdentifier = OLA_IDENTIFIER; // olaIdentifier **must** be the second entry
  int nextSerialLogNumber = 1;
  int nextDataLogNumber = 1;
  //uint32_t: Largest is 4,294,967,295 or 4,294s or 71 minutes between readings.
  //uint64_t: Largest is 9,223,372,036,854,775,807 or 9,223,372,036,854s or 292,471 years between readings.
  uint64_t usBetweenReadings = 100000ULL; //Default to 100,000us = 100ms = 10 readings per second.
  //100,000 / 1000 = 100ms. 1 / 100ms = 10Hz
  //recordPerSecond (Hz) = 1 / ((usBetweenReadings / 1000UL) / 1000UL)
  //recordPerSecond (Hz) = 1,000,000 / usBetweenReadings
  bool logMaxRate = false;
  bool enableRTC = true;
  bool enableIMU = true;
  bool enableTerminalOutput = true;
  bool logDate = true;
  bool logTime = true;
  bool logData = true;
  bool logSerial = true;
  bool logIMUAccel = true;
  bool logIMUGyro = true;
  bool logIMUMag = true;
  bool logIMUTemp = true;
  bool logRTC = true;
  bool logHertz = true;
  bool correctForDST = false;
  int dateStyle = 0; // 0 : mm/dd/yyyy, 1 : dd/mm/yyyy, 2 : yyyy/mm/dd, 3 : ISO8601 (date and time)
  bool hour24Style = true;
  int  serialTerminalBaudRate = 115200;
  int  serialLogBaudRate = 9600;
  bool showHelperText = true;
  bool logA11 = false;
  bool logA12 = false;
  bool logA13 = false;
  bool logA32 = false;
  bool logAnalogVoltages = true;
  float localUTCOffset = 0; // Default to UTC because we should. Support offsets in 15 minute increments.
  bool printDebugMessages = false;
#if(HARDWARE_VERSION_MAJOR == 0)
  bool powerDownQwiicBusBetweenReads = false; // For the SparkX (black) board: default to leaving the Qwiic power enabled during sleep and powerDown to prevent a brown-out.
#else
  bool powerDownQwiicBusBetweenReads = true; // For the SparkFun (red) board: default to disabling Qwiic power during sleep. (Qwiic power is always disabled during powerDown on v10 hardware.)
#endif
  uint32_t qwiicBusMaxSpeed = 100000; // 400kHz with no pull-ups can cause issues. Default to 100kHz. User can change to 400 if required.
  int  qwiicBusPowerUpDelayMs = 250; // This is the minimum delay between the qwiic bus power being turned on and communication with the qwiic devices being attempted
  bool printMeasurementCount = false;
  bool enablePwrLedDuringSleep = true;
  bool logVIN = false;
  unsigned long openNewLogFilesAfter = 0; //Default to 0 (Never) seconds
  float vinCorrectionFactor = 1.47; //Correction factor for the VIN measurement; to compensate for the divider impedance
  bool useGPIO32ForStopLogging = false; //If true, use GPIO as a stop logging button
  uint32_t qwiicBusPullUps = 1; //Default to 1.5k I2C pull-ups - internal to the Artemis
  bool outputSerial = false; // Output the sensor data on the TX pin
  uint8_t zmodemStartDelay = 20; // Wait for this many seconds before starting the zmodem file transfer
  bool enableLowBatteryDetection = false; // Low battery detection
  float lowBatteryThreshold = 3.4; // Low battery voltage threshold (Volts)
  bool frequentFileAccessTimestamps = false; // If true, the log file access timestamps are updated every 500ms
  bool useGPIO11ForTrigger = false; // If true, use GPIO to trigger sensor logging
  bool fallingEdgeTrigger = true; // Default to falling-edge triggering (If false, triggering will be rising-edge)
  bool imuAccDLPF = false; // IMU accelerometer Digital Low Pass Filter - default to disabled
  bool imuGyroDLPF = false; // IMU gyro Digital Low Pass Filter - default to disabled
  int imuAccFSS = 0; // IMU accelerometer full scale - default to gpm2 (ICM_20948_ACCEL_CONFIG_FS_SEL_e)
  int imuAccDLPFBW = 7; // IMU accelerometer DLPF bandwidth - default to acc_d473bw_n499bw (ICM_20948_ACCEL_CONFIG_DLPCFG_e)
  int imuGyroFSS = 0; // IMU gyro full scale - default to 250 degrees per second (ICM_20948_GYRO_CONFIG_1_FS_SEL_e)
  int imuGyroDLPFBW = 7; // IMU gyro DLPF bandwidth - default to gyr_d361bw4_n376bw5 (ICM_20948_GYRO_CONFIG_1_DLPCFG_e)
  bool logMicroseconds = false; // Log micros()
  bool useTxRxPinsForTerminal = false; // If true, the terminal is echo'd to the Tx and Rx pins. Note: setting this to true will _permanently_ disable serial logging and analog input on those pins!
  bool timestampSerial = false; // If true, the RTC time will be added to the serial log file when timeStampToken is received
  uint8_t timeStampToken = 0x0A; // Add RTC time to the serial log when this token is received. Default to Line Feed (0x0A). Suggested by @DennisMelamed in Issue #63
  bool useGPIO11ForFastSlowLogging = false; // If true, Pin 11 will control if readings are taken quickly or slowly. Suggested by @ryanneve in Issue #46 and PR #64
  bool slowLoggingWhenPin11Is = false; // Controls the polarity of Pin 11 for fast / slow logging
  bool useRTCForFastSlowLogging = false; // If true, logging will be slow during the specified times
  int slowLoggingIntervalSeconds = 300; // Slow logging interval in seconds. Default to 5 mins
  int slowLoggingStartMOD = 1260; // Start slow logging at this many Minutes Of Day. Default to 21:00
  int slowLoggingStopMOD = 420; // Stop slow logging at this many Minutes Of Day. Default to 07:00
  bool resetOnZeroDeviceCount = false; // A work-around for I2C bus crashes. Enable this via the debug menu.
  bool imuUseDMP = false; // If true, enable the DMP
  bool imuLogDMPQuat6 = true; // If true, log INV_ICM20948_SENSOR_GAME_ROTATION_VECTOR (Quat6)
  bool imuLogDMPQuat9 = false; // If true, log INV_ICM20948_SENSOR_ROTATION_VECTOR (Quat9 + Heading Accuracy)
  bool imuLogDMPAccel = false; // If true, log INV_ICM20948_SENSOR_RAW_ACCELEROMETER
  bool imuLogDMPGyro = false; // If true, log INV_ICM20948_SENSOR_RAW_GYROSCOPE
  bool imuLogDMPCpass = false; // If true, log INV_ICM20948_SENSOR_MAGNETIC_FIELD_UNCALIBRATED
  unsigned long minimumAwakeTimeMillis = 0; // Set to greater than zero to keep the Artemis awake for this long between sleeps
  bool identifyBioSensorHubs = false; // If true, Bio Sensor Hubs (Pulse Oximeters) will be included in autoDetect (requires exclusive use of pins 32 and 11)
  bool serialTxRxDuringSleep = false; // If true, the Serial Tx and Rx pins are left enabled during sleep - to prevent the COM port reinitializing
  bool printGNSSDebugMessages = false;
  uint8_t serialNumber = 0; // OW
} settings;

//These are the devices on board OpenLog that may be on or offline.
struct struct_online {
  bool microSD = false;
  bool dataLogging = false;
  bool serialLogging = false;
  bool IMU = false;
  bool serialOutput = false;
} online;
