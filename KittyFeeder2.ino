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
#include "InputHandler.h"
#include "Button.h"
#include <MenuSystem.h>
#include "StorageMenu.h"
// Have to use this library due to conflicts with Servo interrupts
#include "SoundPlayer.h"
#include <LiquidCrystal.h>
#include "ESP8266.h"

char err_buf[ERROR_BUF_SIZE];
uint16_t err_remain;
#include "FeederUtils.h"

///// ALL THE DEVICE CONFIGS COME FROM HERE
#include "FeederConfig.h"
#define ARROW_CHAR ((uint8_t)0)

// Locations for the arrows on the feed menus
const uint8_t feedMenuArrowLocs[] = {0, 4, 8, 11};

// Watchdog Timer
void wdtService(); bool wdtOn(); void wdtOff();

void serviceFeeds();
void serviceCooler();
void serviceSerial();
void servicePiezo();
void serviceWifi();
double getTemp();
void inputHandler();

bool anyBtnWasPressed();
bool anyBtnIsPressed();

void displayIdleMenu(Menu *cp_menu);
void displayMenu(Menu *cp_menu);
void displayFeed1(Menu *cp_menu);
void displayFeed2(Menu *cp_menu);
// Helper function
void displayFeed(const uint8_t index, StorageMenu *cp_menu);
void displayTemp(Menu *cp_menu);
void displaySystemInfo(Menu *cp_menu);

uint8_t calcLcdTitleCenter(const char* str);

//wifi
ESP8266 wifi(Serial1, 115200);
//piezo
SoundPlayer piezo(PIEZO_PIN1, PIEZO_PIN2);
// Current input handler
InputHandler currHandler;
//buttons
Button bRight(BTN_PIN_RIGHT, true, true, BTN_DEBOUNCE_TIME);
Button bUp(BTN_PIN_UP, true, true, BTN_DEBOUNCE_TIME);
Button bDown(BTN_PIN_DOWN, true, true, BTN_DEBOUNCE_TIME);
Button bLeft(BTN_PIN_LEFT, true, true, BTN_DEBOUNCE_TIME);
Button bSelect(BTN_PIN_SELECT, true, true, BTN_DEBOUNCE_TIME);

//put them into array to service laver
Button *const bAll[] = { &bSelect, &bLeft, &bRight, &bUp, &bDown };

// Declare all your feed compartments and link them with servos
FeedCompart feeds[] = {
  FeedCompart(piezo, SERVO1_PIN, EEPROM_FEEDER_SETTING_LOC, SERVO1_CLOSE, SERVO1_OPEN),
  FeedCompart(piezo, SERVO2_PIN, EEPROM_FEEDER_SETTING_LOC + FEED_COMPART_EE_SIZE, SERVO2_CLOSE, SERVO2_OPEN),
};

ThermoCooler cooler(THERMO_COOLER_PIN, &getTemp, EEPROM_COOLER_SETTINGS_LOC);

// Menu variables
MenuSystem ms;
Menu mm_idle("Idle Menu", &displayIdleMenu);
Menu mm("KittyFeeder v2.0", &displayMenu);
Menu mm_feeds("Feeders", &displayMenu);
// Menu storage struct to track state
FeedMenuStorage fm1 = {.arrow_locs = feedMenuArrowLocs,
                       .num_locs = sizeof(feedMenuArrowLocs) / sizeof(feedMenuArrowLocs[0]),
                       .curr_loc = 0
                      },
                fm2 = {.arrow_locs = feedMenuArrowLocs,
                       .num_locs = sizeof(feedMenuArrowLocs) / sizeof(feedMenuArrowLocs[0]),
                       .curr_loc = 0
                      };

StorageMenu feeds_feed1("Left Feeder", &fm1, sizeof(fm1), &displayFeed1);
StorageMenu feeds_feed2("Right Feeder", &fm2, sizeof(fm2), &displayFeed2);

Menu mm_temp("Cooler", &displayTemp);
Menu mm_clock("Clock");
Menu mm_wifi("Wifi");
Menu mm_sys_info("About", &displaySystemInfo);

// Pick pins without any special functionality
LiquidCrystal lcd(22, 23, 24, 25, 26, 27);
Scheduler ts;

//////// TASKS /////////////
Task tWatchdog(500, TASK_FOREVER, &wdtService, &ts, false, &wdtOn, &wdtOff);
Task tServiceFeeds(TASK_IMMEDIATE, TASK_FOREVER, &serviceFeeds, &ts, true);
Task tServiceCooler(2000, TASK_FOREVER, &serviceCooler, &ts, true);
Task tServiceInput(TASK_IMMEDIATE, TASK_FOREVER, &inputHandler, &ts, true);
Task tServiceSerial(1, TASK_FOREVER, &serviceSerial, &ts, true);
Task tServicePiezo(TASK_IMMEDIATE, TASK_FOREVER, &servicePiezo, &ts, true);
Task tServiceWifi(100, TASK_FOREVER, &serviceWifi, &ts, true); // dont run as often because too exspensive with cpu time

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
  for (int i = 0; i < sizeof(feeds) / sizeof(feeds[0]); i++)
  {
    feeds[i].begin();
  }
  cooler.begin();
  lcd.createChar(ARROW_CHAR, arrowChar);
  lcd.begin(16, LCD_ROWS);

  LOG(LOG_DEBUG, "Building LCD menu tree...");
  mm_idle.add_menu(&mm);
  mm.add_menu(&mm_feeds);
  mm_feeds.add_menu(&feeds_feed1);
  mm_feeds.add_menu(&feeds_feed2);
  mm.add_menu(&mm_temp);
  mm.add_menu(&mm_clock);
  mm.add_menu(&mm_wifi);
  mm.add_menu(&mm_sys_info);

  ms.set_root_menu(&mm_idle);

  // Set input handler
  currHandler = IdleMenuHandler;
  LOG(LOG_DEBUG, "Done building LCD menu tree");
  ms.display();

  if (wifi.setOprToSoftAP() && wifi.setSoftAPParam(SSID, PASSWORD)
      && wifi.enableMUX() && wifi.startTCPServer(80) && wifi.setTCPServerTimeout(10)) {
    LOG(LOG_DEBUG, "Success creating AP SSID: '%s', PASS: '%s'", SSID, PASSWORD);
  } else {
    LOG(LOG_ERROR, "Failed to create wifi AP");
  }

  LOG(LOG_DEBUG, "Startup complete! Starting tasks....\n");
  piezo.play(&SoundPlayer::boot);
  tWatchdog.enableDelayed();

  //teporary crap
}

void loop() {
  ts.execute();
}

void displayFeed1(Menu *cp_menu)
{
  //temporarily stop servicing feeds
  if (currHandler != Feeder1MenuHandler) LOG(LOG_DEBUG, "Entering feed menu, disabling feed servicing");
  tServiceFeeds.disable();
  currHandler = Feeder1MenuHandler;
  displayFeed(0, (StorageMenu *)cp_menu);
}

void displayFeed2(Menu *cp_menu)
{
  if (currHandler != Feeder2MenuHandler) LOG(LOG_DEBUG, "Entering feed menu, disabling feed servicing");
  tServiceFeeds.disable();
  currHandler = Feeder2MenuHandler;
  displayFeed(1, (StorageMenu *)cp_menu);

}

void displayFeed(const uint8_t index, StorageMenu *cp_menu)
{
  FeedCompart curr = feeds[index];
  currHandler = (index) ? Feeder2MenuHandler : Feeder1MenuHandler;
  FeedMenuStorage *stor = (FeedMenuStorage *)cp_menu->getStorage();
  const char lf[] = "Left Feeder";
  const char rf[] = "Right Feeder";
  const char *name = index ? rf : lf;

  lcd.clear();
  lcd.setCursor(calcLcdTitleCenter(name), 0);
  lcd.print(name);

  lcd.setCursor(0, 1);
  lcd.print(curr.isEnabled() ? " On  " : " Off ");
  lcd.print(dayShortStr(curr.getWeekDay()));
  lcd.print(' ');
  if (curr.getHour() < 10) lcd.print(0);
  lcd.print(curr.getHour());
  lcd.print(":");
  if (curr.getMin() < 10) lcd.print(0);
  lcd.print(curr.getMin());

  lcd.setCursor(stor->arrow_locs[stor->curr_loc], 1);
  lcd.write(ARROW_CHAR);
}

void displayMenu(Menu *cp_menu) {

  currHandler = MenuNavigatorHandler;
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
    curr == i ? lcd.write(ARROW_CHAR) : lcd.write(' ');
    lcd.print(mi->get_name());
  }

}

void displayTemp(Menu *cp_menu)
{
  char str[25];
  currHandler = TemperatureMenuHandler;
  lcd.clear();
  snprintf(str, sizeof(str), "%s %d%cF %d%%", cp_menu->get_name(),
           (int)round(cooler.getTemp()), 0xDF, cooler.getPwmPercent());
  lcd.setCursor(calcLcdTitleCenter(str), 0);
  lcd.print(str);


  snprintf(str, sizeof(str), "%d%cF", cooler.getSetTemp(), 0xDF);
  lcd.setCursor(calcLcdTitleCenter(str), 1);
  lcd.print(str);
  lcd.setCursor(calcLcdTitleCenter(str) - 1, 1);
  lcd.write(ARROW_CHAR);

}

void displaySystemInfo(Menu *cp_menu)
{
  currHandler = MenuNavigatorHandler;
  char str[] = "Version: "VERSION;
  lcd.clear();
  lcd.setCursor(calcLcdTitleCenter(str), 0);
  lcd.print(str);
  lcd.setCursor(0, 1);
  lcd.print("By Aaron Wisner");
}

void displayIdleMenu(Menu *cp_menu)
{
  currHandler = IdleMenuHandler;
  char str[17];
  lcd.clear();
  //Format the time string
  char mstr[5];
  char sstr[5];
  const char ltt[] = "0%d";
  const char gtet[] = "%d";
  int mins = minute();
  int secs = second();
  snprintf(mstr, sizeof(mstr), mins < 10 ? ltt : gtet, mins);
  snprintf(sstr, sizeof(sstr), secs < 10 ? ltt : gtet, secs);
  snprintf(str, sizeof(str), "%s %d:%s:%s", dayShortStr(weekday()), hour(), mstr, sstr);
  lcd.setCursor(calcLcdTitleCenter(str), 0);
  lcd.print(str);

  //Second line
  snprintf(str, sizeof(str), "1:%s %d%% 2:%s", feeds[0].isEnabled() ? "On" : "Off",
           cooler.getPwmPercent(), feeds[1].isEnabled() ? "On" : "Off");
  lcd.setCursor(calcLcdTitleCenter(str), 1);
  lcd.print(str);
}


void serviceSerial() {
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

void servicePiezo()
{
  piezo.service();
}

bool anyBtnWasPressed()
{
  for (int i = 0; i < sizeof(bAll) / sizeof(bAll[0]); i++)
  {
    // Some weird reason where they are Null
    if (bAll[i]->wasPressed()) return true;
  }
  return false;
}

bool anyBtnIsPressed()
{
  for (int i = 0; i < sizeof(bAll) / sizeof(bAll[0]); i++)
  {
    // Some weird reason where they are Null
    if (bAll[i]->isPressed()) return true;
  }
  return false;
}

void serviceButtons()
{
  for (int i = 0; i < sizeof(bAll) / sizeof(bAll[0]); i++)
  {
    // Some weird reason where they are Null
    if (bAll[i]) bAll[i]->read();
  }

  if (anyBtnWasPressed()) {
    piezo.click();
    const char *str = "button pressed";
    if (bSelect.wasPressed()) LOG(LOG_DEBUG, "Select %s", str);
    if (bRight.wasPressed()) LOG(LOG_DEBUG, "Right %s", str);
    if (bLeft.wasPressed()) LOG(LOG_DEBUG, "Left %s", str);
    if (bUp.wasPressed()) LOG(LOG_DEBUG, "Up %s", str);
    if (bDown.wasPressed()) LOG(LOG_DEBUG, "Down %s", str);
  }
}

void serviceFeeds()
{
  bool enCooler = false;
  for (int i = 0; i < sizeof(feeds) / sizeof(feeds[0]); i++)
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

void serviceWifi()
{
  static int lastMux = -1;
  char buffer[200];
  uint8_t mux_id;
  uint32_t len = wifi.recv(&mux_id, (uint8_t*)buffer, sizeof(buffer), 50);

  // Otherwise service old client
  if (len > 0 && lastMux != mux_id) {
    LOG(LOG_DEBUG, "Wifi got client id:'%d'", mux_id);
    char rply[] PROGMEM =
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/html\r\n"
      "Connection: close\r\n"
      "\r\n"      
      "<!DOCTYPE html>"
      "<html>"
      "<head>"
      "<meta charset=\"UTF-8\">"
      "<title>Kitty Feeder 2K</title>"
      "</head>"
      "<body>"
      "<h1>Kitty Feeder 2K Web Interface!</h1>"
      "<p style='color: red'>Note that this web interface is under development</p>"
      "<p>You are running: '" VERSION 
      "', visit <a href='https://github.com/wizard97/KittyFeeder2'>the GitHub repo for firware updates</a>";

    // Send data
    wifi.send(mux_id, (uint8_t *)rply, sizeof(rply));

    //Format the time string
    char mstr[5];
    char sstr[5];
    const char ltt[] = "0%d";
    const char gtet[] = "%d";
    int mins = minute();
    int secs = second();
    snprintf(mstr, sizeof(mstr), mins < 10 ? ltt : gtet, mins);
    snprintf(sstr, sizeof(sstr), secs < 10 ? ltt : gtet, secs);
    
    int buflen = MIN(snprintf((char*)buffer, sizeof(buffer), 
        "<p>System Time: %s %d:%s:%s (uptime: %d mins)</p>", 
        dayShortStr(weekday()), hour(), mstr, sstr, millis()/60000), sizeof(buffer)-1);
    wifi.send(mux_id, (uint8_t *)buffer, buflen);
        
    buflen = MIN(snprintf((char*)buffer, sizeof(buffer), 
        "<p>Cooler: %dF (set: %dF) (%d%%)</p>", 
        (int)round(cooler.getTemp()), cooler.getSetTemp(), cooler.getPwmPercent()), sizeof(buffer)-1);

    wifi.send(mux_id, (uint8_t *)buffer, buflen);

    buflen = MIN(snprintf((char*)buffer, sizeof(buffer), 
        "<p>Feed #1: %s (%s, %d:%d), Feed #2: %s (%s, %d:%d) </p><body></html>", 
        feeds[0].isEnabled() ? "On" : "Off", dayShortStr(feeds[0].getWeekDay()), feeds[0].getHour(), feeds[0].getMin(),
        feeds[1].isEnabled() ? "On" : "Off", dayShortStr(feeds[1].getWeekDay()), feeds[1].getHour(), feeds[1].getMin()),
        sizeof(buffer)-1);

    wifi.send(mux_id, (uint8_t *)buffer, buflen);

    if (wifi.releaseTCP(mux_id)) {
      LOG(LOG_DEBUG, "Released client id: '%d'", mux_id);
    } else {
      LOG(LOG_ERROR, "Error releasing client id: '%d'", mux_id);
    }
    lastMux = mux_id;
  } else if (len <= 0){
    lastMux = -1;
  }
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

uint8_t calcLcdTitleCenter(const char* str)
{
  uint8_t len = strlen(str);
  return max(0, 7 - (len - 1) / 2);

}

// Pin change interrupt for buttons
ISR (PCINT1_vect)
{
  serviceButtons();
}

bool wdtOn() {

  //disable interrupts
  cli();
  //reset watchdog
  wdt_reset();
  //set up WDT interrupt
  WDTCSR = (1 << WDCE) | (1 << WDE);
  //Start watchdog timer with aDelay prescaller
  WDTCSR = (1 << WDIE) | (1 << WDE) | (WDTO_2S & 0x2F);
  //  WDTCSR = (1<<WDIE)|(WDTO_2S & 0x2F);  // interrupt only without reset
  //Enable global interrupts
  sei();
}

/**
   This On Disable method disables WDT
*/
void wdtOff()
{
  wdt_disable();
}

/**
   This is a periodic reset of WDT
*/
void wdtService()
{
  wdt_reset();
}

/**
   Watchdog timeout ISR

*/
ISR(WDT_vect)
{
  Task& T = ts.currentTask();

  digitalWrite(13, HIGH);
  EEPROM.write(EEPROM_WDT_DEBUG_LOC, (byte)T.getId());
  EEPROM.write(EEPROM_WDT_DEBUG_LOC + 1, (byte)T.getControlPoint());
  digitalWrite(13, LOW);
}
