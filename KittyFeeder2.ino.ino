#define _TASK_WDT_IDS
#include <TaskScheduler.h>

#include <EEPROM.h>
#include <avr/wdt.h>
#include <string.h>

#define VERSION "2.0"

#ifdef NDEBUG
#define DEBUG(M, ...)
#define ERROR(M, ...)
#else
char error_buf[50];
#define DEBUG(M, ...) \
  snprintf(error_buf, sizeof(error_buf), "DEBUG %d: " M, __LINE__, ##__VA_ARGS__); \
  Serial.println(error_buf);

#define ERROR(M, ...) \
  snprintf(error_buf, sizeof(error_buf), "ERROR %d: " M, __LINE__, ##__VA_ARGS__); \
  Serial.println(error_buf);
#endif

// Watchdog Timer
void wdtService(); bool wdtOn(); void wdtoff();

Scheduler ts;


void test() {
  Serial.println("Freezing!!!");
  
  unsigned int start = millis();
  for (int i=0; true ; i++)
  {
    Serial.print("Stuck: ");
    Serial.print(millis()-start);
    Serial.println(" secs");
    delay(100);
  }
  
}
//////// TASKS /////////////
Task tWatchdog(500, TASK_FOREVER, &wdtService, &ts, false, &wdtOn, &wdtOff);
Task tTest(TASK_SECOND, TASK_FOREVER, &test, &ts, true);

void setup() {
  Serial.begin(115200);
  DEBUG("Welcome to the KittyFeeder " VERSION);
  
  if (EEPROM.read(0))
  {
    ERROR("Watchdog triggered reset for task: %u", EEPROM.read(0));
    ERROR("Control Point: %u", EEPROM.read(1));
    EEPROM.write(0, 0);
    EEPROM.write(1, 0);
  }
  
  delay(2000);

  
  tWatchdog.enableDelayed();

}

void loop() {
ts.execute();
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
void wdtOff() {
  wdt_disable();
}

/**
 * This is a periodic reset of WDT
 */
void wdtService() {
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
  EEPROM.write(0, (byte)T.getId());
  EEPROM.write(1, (byte)T.getControlPoint());
  digitalWrite(13, LOW);
}


