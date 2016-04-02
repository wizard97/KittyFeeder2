#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include "Arduino.h"
#include "Button.h"
#include "MenuSystem.h"

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
    uint8_t wday;
    if (bLeft.wasPressed())
    {
        feeds[index].saveSettingsToEE();
        ms.back();
    }

    // Weekday selector
    wday = feeds[index].getWeekDay();
    if (bUp.wasPressed())  wday = (wday == 7) ? wday=1 : (wday+1);
    else if (bDown.wasPressed()) wday = (wday==1) ? wday=7 : (wday-1);
    feeds[index].setWeekDay(wday);

}

#endif
