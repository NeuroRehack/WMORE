

#pragma once

// Flags for output destinations

#define OL_OUTPUT_SERIAL   	0x1
#define OL_OUTPUT_SDCARD	0x2

void printHelperText(uint8_t);
void getData(void);
uint8_t getByteChoice(int numberOfSeconds, bool updateDZSERIAL = false);
void menuMain(void);
void beginSD(void);
void beginIMU(void);
