#ifndef STORAGE_MENU_H
#define STORAGE_MENU_H

#include "MenuSystem.h"

// This class is a sub class of Menu defined in MenuSystem.h
// It hold a pointer to store whatever you want for each menu
class StorageMenu : public Menu
{
public:
    void* getStorage()
    {
        return _stor;
    }

    void* setStorage(void *stor, uint16_t len)
    {
        _stor = stor;
        _stor_len = len;
    }


    StorageMenu(const char* name, void (*callback)(Menu*) = NULL)
    : Menu(name, callback)
    {
        _stor = NULL;
        _stor_len = 0;
    }

    StorageMenu(const char* name, void *stor, uint16_t len, void (*callback)(Menu*) = NULL)
    : Menu(name, callback)
    {
        _stor = stor;
        _stor_len = len;
    }

private:
    void *_stor;
    uint16_t _stor_len;
};
#endif
