#ifndef __BUG_FIX_H__
#define __BUG_FIX_H__

// ---------------------------------------------------------------- lowPower.ino
void checkBattery(void);

void powerDownOLA(void);

void resetArtemis(void);

void goToSleep(uint32_t sysTicksToSleep);

void wakeFromSleep();

void stopLogging(void);

void waitForQwiicBusPowerDelay();

void qwiicPowerOn();

void qwiicPowerOff();

void microSDPowerOn();

void microSDPowerOff();

void imuPowerOn();

void imuPowerOff();

void powerLEDOn();

void powerLEDOff();

uint64_t rtcMillis();

int calculateDayOfYear(int day, int month, int year);

// ---------------------------------------------------------------- support.ino
bool useRTCmillis(void);

uint64_t bestMillis(void);

void printDebug(String thingToPrint);

void printUnknown(uint8_t unknownChoice);

void printUnknown(int unknownValue);

void waitForInput();

uint8_t getByteChoice(int numberOfSeconds, bool updateDZSERIAL);

int64_t getNumber(int numberOfSeconds);

double getDouble(int numberOfSeconds);

static uint64_t divu64_10(uint64_t ui64Val);

static int uint64_to_str(uint64_t ui64Val, char *pcBuf);

static int olaftoa(float fValue, char *pcBuf, int iPrecision, int bufSize);



#endif