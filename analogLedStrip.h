#ifndef ANALOGLEDSTRIP_H
#define ANALOGLEDSTRIP_H

#include <Arduino.h>
#include "fadeColor.h"
#include "stripcontrol.h"

#define REDPIN 5
#define GREENPIN 4
#define BLUEPIN 2

// setup pin related things.
void setupAnalogStrip();
/*
    write a red green blue value,
    corresponding to it's brightness,
    with value range 0-255.
*/
void writeRgb(int, int, int);
void fadeRgb(int, int);

#endif