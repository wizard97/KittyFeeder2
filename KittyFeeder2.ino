#include <DS1307RTC.h>
#include <TimeLib.h>
#include <EEPROM.h>
#include <avr/wdt.h>
#include <string.h>

#define _TASK_WDT_IDS
#include <TaskScheduler.h>
#include "FeedCompart.h"

char error_buf[ERROR_BUF_SIZE];
#include "FeederUtils.h"

#define VERSION "v2.0"
#define RTC_SYNC_INTERVAL 30

#define SERVO1_PIN 9
#define SERVO1_OPEN 90
#define SERVO1_CLOSE 0

#define SERVO2_PIN 10
#define SERVO2_OPEN 0
#define SERVO2_CLOSE 90

// Requires two bytes from this index
#define EEPROM_WDT_DEBUG_LOC (EEPROM.length()-3)


// Watchdog Timer
void wdtService(); bool wdtOn(); void wdtOff();

void serviceFeeds();

// Declare all your feed compartments and link them with servos

FeedCompart feeds[] = {
  FeedCompart(SERVO1_PIN, 0, SERVO1_CLOSE, SERVO1_OPEN),
  FeedCompart(SERVO2_PIN, sizeof(FeedCompart), SERVO2_CLOSE, SERVO2_OPEN),
  };


Scheduler ts;

//////// TASKS /////////////
Task tWatchdog(500, TASK_FOREVER, &wdtService, &ts, false, &wdtOn, &wdtOff);
Task tServiceFeeds(TASK_IMMEDIATE, TASK_FOREVER, &serviceFeeds, &ts, true);

void setup() {
  Serial.begin(115200);
  setTime(1);
  setSyncProvider(&RTC.get);  // set the external time provider
  setSyncInterval(RTC_SYNC_INTERVAL);
  
  DEBUG("Welcome to the KittyFeeder " VERSION);
  
  if (timeStatus() != timeSet) 
     ERROR("Unable to sync with the RTC");
  else
     DEBUG("RTC has set the system time");      
  
  if (EEPROM.read(EEPROM_WDT_DEBUG_LOC))
  {
    ERROR("Watchdog triggered system reset for task_id: %u (cp: %d)", 
      EEPROM.read(EEPROM_WDT_DEBUG_LOC), EEPROM.read(EEPROM_WDT_DEBUG_LOC + 1));
    EEPROM.write(EEPROM_WDT_DEBUG_LOC, 0);
    EEPROM.write(EEPROM_WDT_DEBUG_LOC + 1, 0);
  }

  for (int i=0; i < sizeof(feeds)/sizeof(feeds[0]); i++)
  {
     feeds[i].begin();
  }
  
  DEBUG("Startup Complete! Starting Tasks...");
  tWatchdog.enableDelayed();
  feeds[0].enable();
}

void loop() {
ts.execute();
}

void serviceFeeds()
{
  for (int i=0; i < sizeof(feeds)/sizeof(feeds[0]); i++)
  {
     feeds[i].service();
  }
  
}

bool wdtOn() {
  
  //disable interrupts
  cli();
  //reset watchdog
  wdt_reset();
  //set up WDT interrupt
  WDTCSR = (1<<WDCE)|(1<<WDE);
  //Start watchdog timer with aDelay prescaller
  WDTCSR = (1<<WDIE)|(1<<WDE)|(WDTO_2S & 0x2F);
//  WDTCSR = (1<<WDIE)|(WDTO_2S & 0x2F);  // interrupt only without reset
  //Enable global interrupts
  sei();
}

/**
 * This On Disable method disables WDT
 */
void wdtOff() 
{
  wdt_disable();
}

/**
 * This is a periodic reset of WDT
 */
void wdtService() 
{
  wdt_reset();
}

/**
 * Watchdog timeout ISR
 * 
 */
ISR(WDT_vect)
{
  Task& T = ts.currentTask();

  digitalWrite(13, HIGH);
  EEPROM.write(EEPROM_WDT_DEBUG_LOC, (byte)T.getId());
  EEPROM.write(EEPROM_WDT_DEBUG_LOC+1, (byte)T.getControlPoint());
  digitalWrite(13, LOW);
}


