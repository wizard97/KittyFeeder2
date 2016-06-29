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
#include <EEPROM.h>
#include "FeederUtils.h"
#include "FeederConfig.h"
#include "SoundPlayer.h"


#define FEED_COMPART_EE_SIZE sizeof(EECompartSettings)


// Make a struct so we can memcpy it out of EEPROM
typedef struct EECompartSettings
{
    boolean enabled;
    //Ignore months and year fields
    uint8_t Minute;
    uint8_t Hour;
    uint8_t Wday;   // day of week, sunday is day 1
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
    SoundPlayer &piezo;
    const uint16_t servoPin;
    const uint16_t eepromLoc;
    // Servo open and close positions
    Servo doorServo;
    const int16_t openDeg, closeDeg;
    uint8_t id;

    EECompartSettings settings;
    DoorState currDoorState;
    // Timestamp of state change for door
    unsigned long msStateChange;
    bool lock;
    static uint8_t _id_counter;

    bool loadSettingsFromEE();
    uint32_t generateCrc();

public:
    FeedCompart(SoundPlayer &piezo, uint16_t servoPin, uint16_t eepromLoc,
        int16_t closeDeg, int16_t openDeg);

    void enable();
    void disable();
    void service();
    void begin();
    void lockFeed() { lock = true; }
    void unlockFeed() { lock = false; }

    // getters
    Servo &getServo();
    bool isEnabled();

    uint8_t getWeekDay() { return settings.Wday; }
    uint8_t getHour() { return settings.Hour; }
    uint8_t getMin() { return settings.Minute; }

    // setters
    void setWeekDay(uint8_t wday) { settings.Wday = wday; }
    void setHour(uint8_t hour) { settings.Hour = hour; }
    void setMin(uint8_t min) { settings.Minute = min; }
    void saveSettingsToEE();

};

//Default it to zero
uint8_t FeedCompart::_id_counter = 0;

FeedCompart::FeedCompart(SoundPlayer &piezo, uint16_t servoPin, uint16_t eepromLoc, int16_t closeDeg, int16_t openDeg)
: piezo(piezo), servoPin(servoPin), eepromLoc(eepromLoc), doorServo(),
openDeg(openDeg), closeDeg(closeDeg), id(++_id_counter)
{
    lock = false;
    currDoorState = CLOSED;
    msStateChange = 0;
}

/*
FeedCompart::~FeedCompart()
{
    doorServo.write(closeDeg);
    doorServo.detach();
}
*/


void FeedCompart::begin()
{
    // If loading settings failed, set to sensible defaults
    if(!loadSettingsFromEE())
    {
        time_t curr = now();
        //Default to now
        settings.Minute = minute(curr);
        settings.Hour = hour(curr);
        settings.Wday = weekday(curr);

        settings.enabled = false;
        saveSettingsToEE();
        LOG(LOG_ERROR,"Feed Door %d: EEPROM corrupt, setting alarm time to now.", id);

    }

    doorServo.attach(servoPin);
    doorServo.write(closeDeg);
    LOG(LOG_DEBUG, "Feed Door %d: Servo attached to pin %d", id, servoPin);

    // No idea why I'm having to do this...
    char tmp[5];
    strcpy(tmp, dayShortStr(settings.Wday));

    LOG(LOG_DEBUG, "Feed Door %d: %s with set time: %s %d:%d",
        id, settings.enabled ? "ENABLED" : "DISABLED", tmp,
        settings.Hour, settings.Minute);

}

void FeedCompart::service()
{
    time_t curr = now();

        // Run the state machine for the door
        switch(currDoorState)
        {
            case CLOSED:
                if ((settings.enabled && !lock && weekday(curr) == settings.Wday)) {
                    // figure out the time_t the door should open
                    tmElements_t to_open;
                    breakTime(curr, to_open);
                    // We know the day is correct, so just fil in the rest
                    to_open.Hour = settings.Hour;
                    to_open.Minute = settings.Minute;
                    to_open.Second = 0;
                    // This is the timestamp it should open
                    time_t set =  makeTime(to_open);

                    //State transition
                    if (curr >= set && curr < set + 60*DOOR_OPEN_TIME) {
                        msStateChange = millis();
                        currDoorState = OPENING;
                        piezo.play(&SoundPlayer::open);
                        LOG(LOG_DEBUG, "Feeder %d opening!", id);

                    }
                }
                doorServo.write(closeDeg);
                break;

            case OPENING:

                if (doorServo.read() != openDeg) {
                    doorServo.write(map(constrain(millis() - msStateChange, 0, DOOR_SPEED), 0,
                        DOOR_SPEED, closeDeg, openDeg));
                } else {
                    msStateChange = millis();
                    LOG(LOG_DEBUG, "Feeder %d opened!", id);
                    currDoorState = OPEN;
                }
                break;

            case OPEN:
                if (millis() - msStateChange > 60000*DOOR_OPEN_TIME && !lock) {
                    msStateChange = millis();
                    currDoorState = CLOSING;
                    piezo.play(&SoundPlayer::close);
                    LOG(LOG_DEBUG, "Feeder %d closing!", id);
                }
                doorServo.write(openDeg);
                break;

            case CLOSING:
                if (doorServo.read() != closeDeg) {
                    doorServo.write(map(constrain(millis() - msStateChange, 0, DOOR_SPEED), 0,
                        DOOR_SPEED, openDeg, closeDeg));
                } else {
                    msStateChange = millis();
                    LOG(LOG_DEBUG, "Feeder %d closed!", id);
                    currDoorState = CLOSED;
                    // disable feed
                    settings.enabled = false;
                    saveSettingsToEE();
                }
                break;

            default:
                LOG(LOG_ERROR, "Feeder %d state machine in invalid state!", id);
                currDoorState = CLOSED;
                doorServo.write(closeDeg);
                break;
        }

}

void FeedCompart::enable()
{
    settings.enabled = true;
}

void FeedCompart::disable()
{
    settings.enabled = false;
}

Servo &FeedCompart::getServo()
{
    return doorServo;
}

bool FeedCompart::isEnabled()
{
    return settings.enabled;
}

bool FeedCompart::loadSettingsFromEE()
{
    // Some crazy pointer casting to perform a memcpy so we can use the EEPROM macro
    for (uint8_t i=0; i<FEED_COMPART_EE_SIZE; i++)
    {
        ((unsigned char*)&settings)[i] = EEPROM[eepromLoc + i];
    }
    settings.Wday = constrain(settings.Wday, 1, 7);
    settings.Hour = constrain(settings.Hour, 0, 23);
    settings.Minute = constrain(settings.Minute, 0, 59);

    return generateCrc() == settings.crc;
}

void FeedCompart::saveSettingsToEE()
{
    uint32_t tmp = generateCrc();

    // update crc
    settings.crc = tmp;
    // Some crazy pointer casting to perform a memcpy so we can use the EEPROM macro
    for (uint8_t i=0; i<sizeof(settings); i++)
    {
        EEPROM[eepromLoc + i] = ((unsigned char*)&settings)[i];
    }
    LOG(LOG_DEBUG, "Feeder settings written to EEPROM");

}

// Code found on Arduino EEPROM example page
uint32_t FeedCompart::generateCrc() {
    //TODO MAKE THIS STUPID THING WORK!!!
    return 0;

    // generate crc, ignoring the last crc element in settings struct
    return EEGenerateCrc(eepromLoc, FEED_COMPART_EE_SIZE-sizeof(settings.crc));
}

#endif
