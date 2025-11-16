#pragma once

#include <stdint.h>   // for uint32_t, uint8_t

// ---- WMORE time structure (global + local UNIX + hundredths) ----
typedef struct {
  uint32_t unix;       // UNIX seconds
  uint8_t  hundredths; // 0–99
} UnixTimeWithHunds;

// Flags for output destinations
#define OL_OUTPUT_SERIAL    0x1
#define OL_OUTPUT_SDCARD    0x2

void printHelperText(uint8_t);
void getData(void);
uint8_t getByteChoice(int numberOfSeconds, bool updateDZSERIAL = false);
void menuMain(void);
void beginSD(void);
void beginIMU(void);

// Optional but nice for clarity:
UnixTimeWithHunds getUnixTimeFromRTC(void);
