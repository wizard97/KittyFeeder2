#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include "Arduino.h"
#include "Button.h"
#include "MenuSystem.h"
#include "StorageMenu.h"
#include "FeederUtils.h"

typedef enum InputHandler
{
    MenuNavigatorHandler,
    Feeder1MenuHandler,
    Feeder2MenuHandler,
    NullHandler
} InputHandler;


// Is there a better way to do this?
extern Button bRight;
extern Button bUp;
extern Button bDown;
extern Button bLeft;
extern Button bSelect;

extern FeedCompart feeds[];
extern MenuSystem ms;
extern InputHandler currHandler;

extern void serviceButtons();
extern bool anyBtnWasPressed();

void inputHandler();
void menuNavigatorHandler();
void feederMenuHandler(const unsigned char index);

void inputHandler()
{
    serviceButtons();
    // which input handler do we use?
    switch (currHandler)
    {
        case MenuNavigatorHandler:
            menuNavigatorHandler();
            break;

        case Feeder1MenuHandler:
            feederMenuHandler(0);
            break;

        case Feeder2MenuHandler:
            feederMenuHandler(1);
            break;

        case NullHandler:
            break;

        default:
            break;
    }
    if (anyBtnWasPressed()) ms.display();
}

void menuNavigatorHandler()
{
    if (bSelect.wasPressed() || bRight.wasPressed()) ms.select(false);
    else if (bLeft.wasPressed()) ms.back();
    else if (bUp.wasPressed()) ms.prev();
    else if (bDown.wasPressed()) ms.next();
}

// Index in feeds array of current item
void feederMenuHandler(const unsigned char index)
{
    StorageMenu *sm= (StorageMenu *)ms.get_current_menu();
    FeedMenuStorage *stor = (FeedMenuStorage *)sm->getStorage();
    uint8_t tmp = 0;


    // Leaving menu
    if (bSelect.wasPressed() || bRight.wasPressed())
    {
        if (stor->curr_loc < 3) {
            stor->curr_loc++;
        } else {
            stor->curr_loc = 0;
            feeds[index].saveSettingsToEE();
            ms.back();
            return;
        }
    } else if (bLeft.wasPressed()) {
        if (stor->curr_loc > 0) {
            stor->curr_loc--;
        } else {
            stor->curr_loc = 0;
            feeds[index].saveSettingsToEE();
            ms.back();
            return;
        }
    }

    // Modifying dates
    if (stor->curr_loc == 0 && (bUp.wasPressed() || bDown.wasPressed())) {
        feeds[index].isEnabled() ? feeds[index].disable() : feeds[index].enable();

    } else if (stor->curr_loc == 1) {
        // Weekday selector
        tmp = feeds[index].getWeekDay();
        if (bUp.wasPressed()) {
            tmp = (tmp == 7) ? tmp=1 : (tmp+1);
            feeds[index].setWeekDay(tmp);
        } else if (bDown.wasPressed()) {
            tmp = (tmp==1) ? tmp=7 : (tmp-1);
            feeds[index].setWeekDay(tmp);
        }

    } else if (stor->curr_loc == 2) {
        //Mofifying Hours
        tmp = feeds[index].getHour();
        if (bUp.wasPressed()) {
            tmp = (tmp == 23) ? tmp=0 : (tmp+1);
            feeds[index].setHour(tmp);
        } else if (bDown.wasPressed()) {
            tmp = (tmp==0) ? tmp=23 : (tmp-1);
            feeds[index].setHour(tmp);
        }

    } else if (stor->curr_loc == 3) {
        //Mofifying Mins
        tmp = feeds[index].getMin();
        if (bUp.wasPressed()) {
            tmp = (tmp == 59) ? tmp=0 : (tmp+1);
            feeds[index].setMin(tmp);
        } else if (bDown.wasPressed()) {
            tmp = (tmp==0) ? tmp=23 : (tmp-1);
            feeds[index].setMin(tmp);
        }

    }

}

#endif
