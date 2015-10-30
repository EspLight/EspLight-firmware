#include "analogLedStrip.h"

static unsigned long ccurrent = 0;
static unsigned long cprevious = 0;
static int cinterval = 10;

static uint8_t *colors = NULL;

void setupAnalogStrip()
{
    // set pwm range to 0 - 255
    analogWriteRange(0xff);
    // initialize pins right.
    pinMode(REDPIN, OUTPUT);
    pinMode(GREENPIN, OUTPUT);
    pinMode(BLUEPIN, OUTPUT);
    digitalWrite(REDPIN, 0);
    digitalWrite(GREENPIN, 0);
    digitalWrite(BLUEPIN, 0);
    yield();
    colors = colorinc();
}

void writeRgb(int rval, int gval, int bval)
{
    analogWrite(REDPIN, rval);
    analogWrite(GREENPIN, gval);
    analogWrite(BLUEPIN, bval);
}

void fadeRgb(int speed, int brightness)
{
    ccurrent = millis();
    if((ccurrent - cprevious) >= cinterval)
    {
        cprevious = ccurrent;
        colors = colorinc();
    }
    cinterval = stripcontrol.varZero+1;
    float brightnessFactor = (float)(((float)stripcontrol.brightness)/100);
    int r = colors[RED] * brightnessFactor;
    int g = colors[GREEN] * brightnessFactor;
    int b = colors[BLUE] * brightnessFactor;
    writeRgb(r, g, b);
}