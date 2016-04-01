#include <DS3232RTC.h> 
#include <TimeLib.h>
#include <EEPROM.h>
#include <avr/wdt.h>
#include <string.h>

#define _TASK_WDT_IDS
#include <TaskScheduler.h>
#include "DHT.h"
#include "FeedCompart.h"
#include "ThermoCooler.h"
#include "Button.h"
#include <MenuSystem.h>
#include <LiquidCrystal.h>

#define LCD_ROWS 2

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

#define THERMO_COOLER_PIN 4

// Give it an interrupt pin, if I ever change to an ainterrupt lib
#define DHTPIN 3
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

//define buttons
#define BTN_DEBOUNCE_TIME 5
#define BTN_PIN_RIGHT A8
#define BTN_PIN_UP A9
#define BTN_PIN_DOWN A10
#define BTN_PIN_LEFT A11
#define BTN_PIN_SELECT A12

#define EEPROM_FEEDER_SETTING_LOC 50
#define EEPROM_COOLER_SETTINGS_LOC 20
// Requires two bytes from this index
#define EEPROM_WDT_DEBUG_LOC (EEPROM.length()-1-2)



typedef enum InputHandler
{
  MenuNavigatorHandler,
  Feeder1MenuHandler,
  NullHandler
} InputHandler;


// Watchdog Timer
void wdtService(); bool wdtOn(); void wdtOff();

void serviceFeeds();
void serviceCooler();
void serviceButtons();
double getTemp();
void inputHandler();

bool anyBtnWasPressed();

void displayMenu(Menu *cp_menu);
void displayFeed1(Menu *cp_menu);
// Current input handler
InputHandler currHandler;
//buttons
Button bRight(BTN_PIN_RIGHT, true, false, BTN_DEBOUNCE_TIME);
Button bUp(BTN_PIN_UP, true, false, BTN_DEBOUNCE_TIME);
Button bDown(BTN_PIN_DOWN, true, false, BTN_DEBOUNCE_TIME);
Button bLeft(BTN_PIN_LEFT, true, false, BTN_DEBOUNCE_TIME);
Button bSelect(BTN_PIN_SELECT, true, false, BTN_DEBOUNCE_TIME);

//put them into array to service laver
Button *const bAll[] = { &bSelect, &bLeft, &bRight, &bUp, &bDown };

// Declare all your feed compartments and link them with servos
FeedCompart feeds[] = {
  FeedCompart(SERVO1_PIN, EEPROM_FEEDER_SETTING_LOC, SERVO1_CLOSE, SERVO1_OPEN),
  FeedCompart(SERVO2_PIN, EEPROM_FEEDER_SETTING_LOC + FEED_COMPART_EE_SIZE, SERVO2_CLOSE, SERVO2_OPEN),
  };

ThermoCooler cooler(THERMO_COOLER_PIN, &getTemp, EEPROM_COOLER_SETTINGS_LOC);

// Menu variables
MenuSystem ms;
Menu mm("KittyFeeder v2.0", &displayMenu);
Menu mm_feeds("Feeders", &displayMenu);
  Menu feeds_feed1("Left Feeder", &displayFeed1);
  Menu feeds_feed2("Right Feeder");

Menu mm_temp("Cooler");
Menu mm_clock("Clock");
Menu mm_wifi("Wifi");

// Pick pins without any special functionality
LiquidCrystal lcd(22, 23, 24, 25, 26, 27);
Scheduler ts;

//////// TASKS /////////////
Task tWatchdog(500, TASK_FOREVER, &wdtService, &ts, false, &wdtOn, &wdtOff);
Task tServiceFeeds(TASK_IMMEDIATE, TASK_FOREVER, &serviceFeeds, &ts, true);
Task tServiceCooler(2000, TASK_FOREVER, &serviceCooler, &ts, true);
Task tServiceInput(TASK_IMMEDIATE, TASK_FOREVER, &inputHandler, &ts, true);

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
  lcd.begin(16, LCD_ROWS);

  LOG(LOG_DEBUG, "Building LCD menu tree...");
  mm.add_menu(&mm_feeds);
    mm_feeds.add_menu(&feeds_feed1);
    mm_feeds.add_menu(&feeds_feed2);
  mm.add_menu(&mm_temp);
  mm.add_menu(&mm_clock);
  mm.add_menu(&mm_wifi);

  ms.set_root_menu(&mm);

  // Set input handler
  currHandler = MenuNavigatorHandler;
  LOG(LOG_DEBUG, "Done building LCD menu tree");
  ms.display();

  LOG(LOG_DEBUG, "Startup complete! Starting tasks....\n");
  tWatchdog.enableDelayed();

  
  //teporary crap
  
  cooler.setTemp(30);
}

void loop() {
ts.execute();
//tmp
serialHandler();
}


void inputHandler()
{
   serviceButtons();
   // which input handler do we use?
   switch (currHandler)
   {
    case MenuNavigatorHandler:
      if (bSelect.wasPressed() || bRight.wasPressed()) ms.select(false);
      else if (bLeft.wasPressed()) ms.back();
      else if (bUp.wasPressed()) ms.prev();
      else if (bDown.wasPressed()) ms.next();
      if (anyBtnWasPressed()) ms.display();
      break;
      
    case Feeder1MenuHandler:
  
      break;
    case NullHandler:
      break;
      
    default:
      break;
   }
}



void displayFeed1(Menu *cp_menu)
{
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Left Feeder");
  lcd.setCursor(0, 1);
  lcd.print('>');
  lcd.print(feeds[0].isEnabled() ? "On  " : "Off ");
  lcd.print(dayShortStr(feeds[0].getWeekDay()));
  lcd.print(' ');
  lcd.print(feeds[0].getHour());
  lcd.print(":");
  lcd.print(feeds[0].getMin());
}

void displayMenu(Menu *cp_menu) {
  lcd.clear();
  lcd.setCursor(0, 0);

  // Display the menu
  byte prev = cp_menu->get_prev_menu_component_num();
  byte curr = cp_menu->get_cur_menu_component_num();
  byte last = cp_menu->get_num_menu_components() - 1;

  byte start, stop;
  MenuComponent const *mi;

  //first
  if (!curr) {
    start = 0;
    stop = min(last, LCD_ROWS - 1);
  } else if (last == curr && last == prev) {
    start = max(0, last + 1 - LCD_ROWS);
    stop = last;
  } else if (curr > prev) {
    //Moved down
    start = prev;
    stop = start + LCD_ROWS - 1;
  } else {
    // going up
    start = curr;
    stop = min(last, start + (LCD_ROWS - 1));
  }

  // draw
  for (int i = start, count = 0; i <= stop; i++, count++)
  {
    lcd.setCursor(0, count);
    mi = cp_menu->get_menu_component(i);
    lcd.print(i + 1);
    curr == i ? lcd.print( ">") : lcd.print(' ');
    lcd.print(mi->get_name());
  }

}

void serialHandler() {
  char inChar;
  if ((inChar = Serial.read()) > 0) {
    switch (inChar) {
      case 'w': // Previus item
        ms.prev();
        ms.display();
        break;
      case 's': // Next item
        ms.next();
        ms.display();
        break;
      case 'a': // Back presed
        ms.back();
        ms.display();
        break;
      case 'd': // Select presed
        ms.select(false);
        ms.display();
        break;
      case '?':
      case 'h': // Display help
        break;
      default:
        break;
    }
  }
}

void serviceButtons()
{
  for (int i=0; i< sizeof(bAll)/sizeof(bAll[0]); i++)
  {
    // Some weird reason where they are Null
    if(bAll[i]) bAll[i]->read(); 
  }
  char *str = "button pressed";
  if (bSelect.wasPressed()) LOG(LOG_DEBUG, "Select %s", str);
  if (bRight.wasPressed()) LOG(LOG_DEBUG, "Right %s", str);
  if (bLeft.wasPressed()) LOG(LOG_DEBUG, "Left %s", str);
  if (bUp.wasPressed()) LOG(LOG_DEBUG, "Up %s", str);
  if (bDown.wasPressed()) LOG(LOG_DEBUG, "Down %s", str);
}

bool anyBtnWasPressed()
{
  for (int i=0; i< sizeof(bAll)/sizeof(bAll[0]); i++)
  {
    // Some weird reason where they are Null
    if(bAll[i]->wasPressed()) return true;
  }
  return false;
}


void serviceFeeds()
{
  bool enCooler = false;
  for (int i=0; i < sizeof(feeds)/sizeof(feeds[0]); i++)
  {
    feeds[i].service();
    enCooler |= feeds[i].isEnabled();
  }
  // Change state of cooler if neccisary
  if (enCooler != cooler.isEnabled()) enCooler ? cooler.enable() : cooler.disable();

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
