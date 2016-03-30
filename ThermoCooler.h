
/*
  ThermoCooler.h - Library for managaing peltier coolers
  Created by D. Aaron Wisner
  Released into the public domain.
*/
#ifndef ThermoCooler_h
#define ThermoCooler_h

#include <Arduino.h>
#include "EEPROM.h"
#include "FeederUtils.h"

// How many degrees from set temp to start using pwm
#define TC_PWM_DELTA_DEG 1
#define THERMO_COOLER_EE_SIZE (sizeof(EEThermoCoolerSettings))

// Make a struct so we can memcpy it out of EEPROM
typedef struct EEThermoCoolerSettings
{
    int set_temp;
    uint32_t crc;
} EEThermoCoolerSettings;

class ThermoCooler
{

public:
    ThermoCooler(int16_t pin, double (*gettemp)(), uint16_t eepromLoc);

    void service();
    double getTemp();
    void disable();
    void enable();
    void begin();
    void setTemp(int temp);
    uint16_t getPwmPercent();
    bool isEnabled();

private:
    bool enabled;
    EEThermoCoolerSettings settings;
    const uint16_t eepromLoc;
    double last_temp;
    const int16_t pin;
    double (*gettemp)();
    uint16_t pwmPercent;

    bool loadSettingsFromEE();
    void saveSettingsToEE();
    uint32_t generateCrc();
};

#endif
