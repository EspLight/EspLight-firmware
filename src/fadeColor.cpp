#include "fadeColor.h"


static uint8_t colors[3] = {0, 0, 0};
static int value = 0;
static int pledselect = 2;
static int ledselect = 0;

uint8_t *colorinc()
{
    value++;
    if(value > 255)
    {
        ledselect++;
        value = 1;
        if(ledselect == 3)
        {
            ledselect = 0;
            pledselect = 2;
        }
        else
        {
            pledselect = ledselect - 1;
        }
    }
    colors[pledselect] = 255 - value;
    colors[ledselect] = value;
    return colors;
}
