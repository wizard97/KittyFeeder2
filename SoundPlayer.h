#ifndef SOUND_PLAYER_H
#define SOUND_PLAYER_H

#include "toneAC2.h"
#include "Arduino.h"

#define MAX_NOTES 20

/*************************************************
 * Public Constants
 *************************************************/

#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978

struct melody
{
    uint16_t durr[MAX_NOTES];
    int notes[MAX_NOTES];
    uint8_t len;
};

class SoundPlayer
{
public:
    static struct melody boot;
    static struct melody open;
    static struct melody close;
    SoundPlayer(int pin1, int pin2)
    {
        _pin1 = pin1;
        _pin2 = pin2;
        _curr = NULL;
        _idx = 0;
    }

    void service()
    {
        // Something to play
        if (_curr != NULL && _curr->durr[_idx] + _start <= millis())
        {
            _idx++;
            // Last element?
            if (_idx >= _curr->len) {
                noToneAC2();
                _curr = NULL;
                _idx = 0;
            } else {
                _start = millis();
                toneAC2(_pin1, _pin2, _curr->notes[_idx], _curr->durr[_idx], true);
            }
        }
    }

    void play(struct melody *m)
    {
        _curr = m;
        _start = millis();
        toneAC2(_pin1, _pin2, m->notes[0], m->durr[0], true);
        _idx = 0;

    }

    void click()
    {
        pinMode(_pin1, OUTPUT);
        pinMode(_pin2, OUTPUT);

        digitalWrite(_pin1, HIGH);
        digitalWrite(_pin2, LOW);

        digitalWrite(_pin1, LOW);
        digitalWrite(_pin2, HIGH);
        delay(1);

        digitalWrite(_pin1, HIGH);
        digitalWrite(_pin2, LOW);
    }

private:
    int _pin1;
    int _pin2;
    const struct melody *_curr;
    unsigned long long _start;
    uint8_t _idx;
};

struct melody SoundPlayer::boot =
    { .durr = {100,100,100,100,100,100,200},
    .notes = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5, NOTE_G4, NOTE_E4, NOTE_C4},
    .len = 7};

    struct melody SoundPlayer::close =
        { .durr = {2000,2000,3000},
        .notes = {NOTE_A4, NOTE_FS4, NOTE_D4},
        .len = 3};

    struct melody SoundPlayer::open =
        { .durr = {2000,2000,3000},
        .notes = {NOTE_D4, NOTE_FS4, NOTE_A4},
        .len = 3};
#endif
