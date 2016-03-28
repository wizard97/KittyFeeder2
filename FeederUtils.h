#include "TimeLib.h"

#ifndef FEEDER_UTILS_H
#define FEEDER_UTILS_H

#define ERROR_BUF_SIZE 90

#ifdef NDEBUG
#define DEBUG(M, ...)
#define ERROR(M, ...)
#else
extern char error_buf[ERROR_BUF_SIZE];
#define DEBUG(M, ...) \
  do { \
    snprintf(error_buf, sizeof(error_buf), "DEBUG (%s %d %d:%d:%d)(%d): " M, \
        monthShortStr(month()), day(), hour(), minute(), second(), __LINE__, ##__VA_ARGS__); \
    Serial.println(error_buf); \
  } while (0)
#define ERROR(M, ...) \
  do { \
    snprintf(error_buf, sizeof(error_buf), "ERROR (%s %d %d:%d:%d)(%d): " M, \
        monthShortStr(month()), day(), hour(), minute(), second(), __LINE__, ##__VA_ARGS__); \
    Serial.println(error_buf); \
  } while (0)
#endif

#endif
