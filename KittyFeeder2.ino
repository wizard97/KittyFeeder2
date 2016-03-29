#include <DS1307RTC.h>
#include <TimeLib.h>
#include <EEPROM.h>
#include <avr/wdt.h>
#include <string.h>

#define _TASK_WDT_IDS
#include <TaskScheduler.h>
#include "DHT.h"
#include "FeedCompart.h"
#include "ThermoCooler.h"

char err_buf[ERROR_BUF_SIZE];
uint16_t err_remain;
#include "FeederUtils.h"

#define VERSION "v2.0"
#define RTC_SYNC_INTERVAL 30

#define SERVO1_PIN 9
#define SERVO1_OPEN 90
#define SERVO1_CLOSE 0

#define SERVO2_PIN 10
#define SERVO2_OPEN 0
#define SERVO2_CLOSE 90

#define THERMO_COOLER_PIN 5

#define DHTPIN 2
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

#define EEPROM_FEEDER_SETTING_LOC 50
#define EEPROM_COOLER_SETTINGS_LOC 20
// Requires two bytes from this index
#define EEPROM_WDT_DEBUG_LOC (EEPROM.length()-1-2)


// Watchdog Timer
void wdtService(); bool wdtOn(); void wdtOff();

void serviceFeeds();
void serviceCooler();
double getTemp();

// Declare all your feed compartments and link them with servos
FeedCompart feeds[] = {
  FeedCompart(SERVO1_PIN, EEPROM_FEEDER_SETTING_LOC, SERVO1_CLOSE, SERVO1_OPEN),
  FeedCompart(SERVO2_PIN, EEPROM_FEEDER_SETTING_LOC + FEED_COMPART_EE_SIZE, SERVO2_CLOSE, SERVO2_OPEN),
  };

ThermoCooler cooler(THERMO_COOLER_PIN, &getTemp, EEPROM_COOLER_SETTINGS_LOC);


Scheduler ts;

//////// TASKS /////////////
Task tWatchdog(500, TASK_FOREVER, &wdtService, &ts, false, &wdtOn, &wdtOff);
Task tServiceFeeds(TASK_IMMEDIATE, TASK_FOREVER, &serviceFeeds, &ts, true);
Task tServiceCooler(2000, TASK_FOREVER, &serviceCooler, &ts, true);

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  setTime(1);
  setSyncProvider(&RTC.get);  // set the external time provider
  setSyncInterval(RTC_SYNC_INTERVAL);

  LOG(LOG_DEBUG, "Welcome to the KittyFeeder " VERSION);


  if (timeStatus() != timeSet)
     LOG(LOG_ERROR, "Unable to sync with the RTC");
  else
     LOG(LOG_DEBUG, "RTC has set the system time");



  if (EEPROM.read(EEPROM_WDT_DEBUG_LOC))
  {
    LOG(LOG_ERROR, "Watchdog triggered system reset for task_id: %u (cp: %d)",
        EEPROM.read(EEPROM_WDT_DEBUG_LOC), EEPROM.read(EEPROM_WDT_DEBUG_LOC + 1));
    EEPROM.write(EEPROM_WDT_DEBUG_LOC, 0);
    EEPROM.write(EEPROM_WDT_DEBUG_LOC + 1, 0);

  }

  // Begin KittyFeeder objects
  for (int i=0; i < sizeof(feeds)/sizeof(feeds[0]); i++)
  {
     feeds[i].begin();
  }
  cooler.begin();

  LOG(LOG_DEBUG, "Startup complete! Starting tasks....\n");
  tWatchdog.enableDelayed();
  cooler.enable();
  cooler.setTemp(72);
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

void serviceCooler()
{
  cooler.service();
  LOG(LOG_DEBUG, "System Temp %dF (%d%%)", (int)round(cooler.getTemp()), cooler.getPwmPercent());

}

double getTemp()
{
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(f)) {
    LOG(LOG_ERROR, "Unable to read temperature sensor");

    return 0.0;
  }
  return f;

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
