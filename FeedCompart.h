/*
  FeederCompartment.h - Class for managing each of the feeding compartments
  Created by D. Aaron Wisner, March 27, 2016.
  Released into the public domain.
*/
#ifndef FEED_COMPART_H
#define FEED_COMPART_H
#include "Arduino.h"
#include "Servo.h"
#include "TimeLib.h"

// Make a struct so we can memcpy it out of EEPROM
typedef struct EECompartSettings
{
    boolean enabled;
    //Ignore months and year fields
    tmElements_t time;
    uint32_t crc;
} FeedCompartSettings;

class FeedCompart
{
private:
    const uint16_t eepromLoc;
    EECompartSettings settings;
    Servo &doorServo;
    // Servo open and close positions
    const uint16_t openDeg, closeDeg;

    bool loadSettingsFromEE();
    void saveSettingsToEE();
    uint32_t generateCrc();

public:
    FeedCompart(Servo &doorServo, uint16_t eepromLoc, uint16_t closeDeg, uint16_t openDeg);
    ~FeedCompart();
};

FeedCompart::FeedCompart(Servo &doorServo, uint16_t eepromLoc, uint16_t closeDeg, uint16_t openDeg)
: doorServo(doorServo), eepromLoc(eepromLoc), openDeg(openDeg), closeDeg(closeDeg)
{
    // If loading settings failed...
    if(!loadSettingsFromEE())
    {
        time_t curr = now();
        //default to now
        settings.time.Second = second(curr);
        settings.time.Minute = minute(curr);
        settings.time.Hour = hour(curr);
        settings.time.Wday = weekday(curr);
        settings.time.Day = day(curr);
        settings.time.Month = month(curr);
        settings.time.Year = year(curr);

        settings.crc = generateCrc();
    }
    doorServo.write(openDeg);
}

FeedCompart::~FeedCompart()
{
    doorServo.write(closeDeg);
    saveSettingsToEE();
}

bool FeedCompart::loadSettingsFromEE()
{
    for (int i=0; i<sizeof(settings); i++)
    {
        ((unsigned char*)&settings)[i] = EEPROM[eepromLoc + i];
    }
    return generateCrc() == settings.crc;
}

void FeedCompart::saveSettingsToEE()
{
    // update crc
    settings.crc = generateCrc();
    for (int i=0; i<sizeof(settings); i++)
    {
        EEPROM[eepromLoc + i] = ((unsigned char*)&settings)[i];
    }
}

uint32_t FeedCompart::generateCrc() {

  const uint32_t crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };

  uint32_t crc = ~0L;
  for (int index = eepromLoc; index < sizeof(settings); index++) {
    crc = crc_table[(crc ^ EEPROM[index]) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (EEPROM[index] >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }
  return crc;
}

#endif
