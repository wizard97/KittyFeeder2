#ifndef FEEDER_UTILS_H
#define FEEDER_UTILS_H

#include "TimeLib.h"
#include "EEPROM.h"
#include "Arduino.h"
#include <avr/pgmspace.h>

#define ERROR_BUF_SIZE 90

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define LOG_DEBUG 0
#define LOG_ERROR 1


#ifdef NDEBUG
#define LOG(T, M, ...)

#else
extern char err_buf[ERROR_BUF_SIZE];
extern uint16_t err_remain;

#define LOG(T, M, ...) \
    do { \
        err_remain = createDebugString(err_buf, sizeof(err_buf), __LINE__, T); \
        snprintf_P(err_buf + err_remain, MAX(0, sizeof(err_buf) - err_remain), \
        PSTR(M), ##__VA_ARGS__); \
        Serial.println(err_buf); \
    } while (0)


#endif


typedef struct FeedMenuStorage
{
  const uint8_t *arrow_locs;
  uint8_t num_locs;
  uint8_t curr_loc;
} FeedMenuStorage;

uint32_t EEGenerateCrc(uint16_t start, uint16_t num_bytes);
uint16_t createDebugString(char *buf, uint16_t buf_size, uint16_t line, bool error);
#endif
