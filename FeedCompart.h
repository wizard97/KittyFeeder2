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


// ms to open and close door
#define DOOR_SPEED 5000
// Mins door should stay open
#define DOOR_OPEN_TIME 15


// Make a struct so we can memcpy it out of EEPROM
typedef struct EECompartSettings
{
    boolean enabled;
    //Ignore months and year fields
    tmElements_t time;
    //crc MUST BE LAST ELEMENT IN STRUCT!
    uint32_t crc;
} FeedCompartSettings;

class FeedCompart
{
    typedef enum DoorState
    {
        CLOSED,
        OPENING,
        OPEN,
        CLOSING,
    } DoorState;

private:
    const uint16_t eepromLoc;
    EECompartSettings settings;
    Servo doorServo;
    DoorState currDoorState;
    // Timestamp of state change for door
    unsigned long msStateChange;
    // Servo open and close positions
    const uint16_t openDeg, closeDeg;
    bool loadSettingsFromEE();
    void saveSettingsToEE();
    uint32_t generateCrc();

public:
    FeedCompart(int servoPin, uint16_t eepromLoc, uint16_t closeDeg, uint16_t openDeg);
    ~FeedCompart();
    Servo &getServo();
    void service();
};

FeedCompart::FeedCompart(int servoPin, uint16_t eepromLoc, uint16_t closeDeg, uint16_t openDeg)
: doorServo(), eepromLoc(eepromLoc),
openDeg(openDeg), closeDeg(closeDeg)
{
    doorServo.attach(servoPin);
    doorServo.write(closeDeg);
    currDoorState = CLOSED;
    msStateChange = 0;

    // If loading settings failed, set to sensible defaults
    if(!loadSettingsFromEE())
    {
        //Default to now
        breakTime(now(), settings.time);
        settings.enabled = false;
        saveSettingsToEE();
    }

}

FeedCompart::~FeedCompart()
{
    doorServo.write(closeDeg);
    doorServo.detach();
    saveSettingsToEE();
}

void FeedCompart::service()
{
    if (settings.enabled) {
        tmElements_t to_check = settings.time;
        time_t curr = now();
        time_t set;
        //since we dont care about day month or year
        to_check.Day = day(curr);
        to_check.Month = month(curr);
        to_check.Year = year(curr);

        set = makeTime(to_check);

        // Run the state machine for the door
        switch(currDoorState)
        {
            case CLOSED:
                if (set >= curr && set < curr + 60*DOOR_OPEN_TIME) {
                    msStateChange = millis();
                    currDoorState = OPENING;
                }
                break;

            case OPENING:
                if (doorServo.read() != openDeg) {
                    doorServo.write(map((long int)(millis - msStateChange), 0,
                        DOOR_SPEED, closeDeg, openDeg));
                } else {
                    msStateChange = millis();
                    currDoorState = OPEN;
                }
                break;

            case OPEN:
                if (curr >= 60*DOOR_OPEN_TIME) {
                    msStateChange = millis();
                    currDoorState = CLOSING;
                }
                break;

            case CLOSING:
                if (doorServo.read() != closeDeg) {
                    doorServo.write(map((long int)(millis - msStateChange), 0,
                        DOOR_SPEED, openDeg, closeDeg));
                } else {
                    msStateChange = millis();
                    currDoorState = CLOSED;
                    // disable feed
                    settings.enabled = false;
                    saveSettingsToEE();
                }
                break;

            default:
                msStateChange = millis();
                currDoorState = CLOSED;
                break;
        }
    }

}

Servo &FeedCompart::getServo()
{
    return doorServo;
}

bool FeedCompart::loadSettingsFromEE()
{
    // Some crazy pointer casting to perform a memcpy so we can use the EEPROM macro
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
    // Some crazy pointer casting to perform a memcpy so we can use the EEPROM macro
    for (int i=0; i<sizeof(settings); i++)
    {
        EEPROM[eepromLoc + i] = ((unsigned char*)&settings)[i];
    }
}

// Code found on Arduino EEPROM example page
uint32_t FeedCompart::generateCrc() {

  const uint32_t crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };

  uint32_t crc = ~0L;
  for (int index = eepromLoc; index < sizeof(settings)-sizeof(settings.crc); index++) {
    crc = crc_table[(crc ^ EEPROM[index]) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (EEPROM[index] >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }
  return crc;
}

#endif
