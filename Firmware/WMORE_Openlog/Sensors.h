#pragma once

#include <stdint.h>   // for uint32_t, uint8_t
#include <stdbool.h>  // for bool (if compiling as C; Arduino C++ also works without)

// ---- WMORE time structure (glocal UNIX + hundredths) ----
typedef struct {
  uint32_t unix;       // UNIX seconds
  uint8_t  hundredths; // 0–99
} UnixTimeWithHunds;

// ---- Sync packet received over UART from coordinator/logger ----
typedef struct {
  uint32_t unix;          // UNIX time (seconds since 1970)
  uint8_t  hundredths;    // hundredths of a second
  bool     valid;         // true if a complete packet was received this cycle
  bool     isCoordinator; // true if this packet came from the coordinator
} SyncPacket;

// Global instance (defined in Sensors.ino)
extern SyncPacket g_syncPacket;

// Flags for output destinations
#define OL_OUTPUT_SERIAL    0x1
#define OL_OUTPUT_SDCARD    0x2

// Public API
void printHelperText(uint8_t);
void getData(void);
uint8_t getByteChoice(int numberOfSeconds, bool updateDZSERIAL = false);
void menuMain(void);
void beginSD(void);
void beginIMU(void);

// Time helpers
UnixTimeWithHunds getUnixTimeFromRTC(void);
bool unixToRTC(uint32_t unixTime,
               uint8_t &sec, uint8_t &min, uint8_t &hour,
               uint8_t &day, uint8_t &month, uint8_t &year2);
uint8_t elapsedMinutes(uint8_t current, uint8_t previous);

// VIN helper
float readVIN(void);