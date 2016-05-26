#ifndef FEEDER_CONFIG_H
#define FEEDER_CONFIG_H

#include "Arduino.h"
#include "EEPROM.h"

byte arrowChar[8] = {
    0b10000,
	0b11000,
	0b11100,
	0b11110,
	0b11110,
	0b11100,
	0b11000,
	0b10000
};

#define VERSION "v2.0"
#define RTC_SYNC_INTERVAL 30

#define PIEZO_PIN1 9
#define PIEZO_PIN2 10

#define SERVO1_PIN 6
#define SERVO1_OPEN 90
#define SERVO1_CLOSE 0

#define SERVO2_PIN 7
#define SERVO2_OPEN 0
#define SERVO2_CLOSE 90

#define THERMO_COOLER_PIN 4
#define MAX_COOLER_SET_TEMP 80
#define MIN_COOLER_SET_TEMP 35

// ms to open and close door
#define DOOR_SPEED 3000
// Mins door should stay open
#define DOOR_OPEN_TIME 3

#define LCD_ROWS 2
// How many ms to redraw menu automatically if no button pressed
#define LCD_AUTO_REDRAW 1000

// Give it an interrupt pin, if I ever change to an ainterrupt lib
#define DHTPIN 3
#define DHTTYPE DHT11   // DHT 22  (AM2302), AM2321

//define buttons
#define BTN_DEBOUNCE_TIME 5
#define BTN_PIN_RIGHT A8
#define BTN_PIN_UP A9
#define BTN_PIN_DOWN A10
#define BTN_PIN_LEFT A11
#define BTN_PIN_SELECT A12

#define EEPROM_FEEDER_SETTING_LOC 0
#define EEPROM_COOLER_SETTINGS_LOC (EEPROM_FEEDER_SETTING_LOC + 2*FEED_COMPART_EE_SIZE)
// Requires two bytes from this index
#define EEPROM_WDT_DEBUG_LOC (EEPROM.length()-1-2)

#endif
