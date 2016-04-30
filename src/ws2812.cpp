#include "ws2812.h"

// holds the length of the ledstrip in pixels or chips.
static int ws2812_striplen;
// create a pointer of NeoPixelBus object type.
NeoPixelBus *strip = NULL;

// timing vairiables.
static unsigned long ccurrent = 0;
static unsigned long cprevious = 0;
static int cinterval = 10;

// variable that holds 
static uint8_t *colors;

/* setup the neopixel object, and clear the ledstrip. */
void setupWS2812(uint16_t length, uint8_t pin)
{
    ws2812_striplen = length;
    colors = colorinc();
    if(strip == NULL)
    {
        strip = new NeoPixelBus(length, pin);
    }
    else
    {
        delete strip;
        strip = new NeoPixelBus(length, pin);
    }
    strip->Begin();
    setWS2812Strip(0, 0, 0);
    strip->Show();
}

/* set the ledstrip to a certain (r, g, b) value. */
void setWS2812Strip(int r, int g, int b)
{
    for(int i = 0; i < ws2812_striplen;i++)
    {
        RgbColor color = RgbColor(r, g, b);
        strip->SetPixelColor(i, color);
    }
}

/* fade the whole ledstrip from one color to another. */
void fadeWS2812(int speed, int brightness)
{
    ccurrent = millis();
    if((ccurrent - cprevious) >= cinterval)
    {
        cprevious = ccurrent;
        colorinc();
    }

    cinterval = speed+1;
    float brightnessFactor = (float)(((float)brightness)/100);
    int r = colors[RED] * brightnessFactor;
    int g = colors[GREEN] * brightnessFactor;
    int b = colors[BLUE] * brightnessFactor;

    setWS2812Strip(r, g, b);
}

/* 
    fade all the pixels individually from one color to the next.
    creating a rainbow like effect.
*/
void rainbowWS2812(int speed, int brightness)
{
    cinterval = speed + 1;
    ccurrent = millis();
    if((ccurrent - cprevious) >= cinterval)
    {
        cprevious = ccurrent;
        static int range = 0xff*3;
        uint8_t buffer[3][ws2812_striplen];
        int i, s;
        colors = colorinc();
        for(i = 0; i < ws2812_striplen; i++)
        {
            for(s = 0; s < range/ws2812_striplen; s++)
                colors = colorinc();
            float brightnessFactor = (float)(((float)brightness) / 100);
            int r = colors[RED] * brightnessFactor;
            int g = colors[GREEN] * brightnessFactor;
            int b = colors[BLUE] * brightnessFactor;
            RgbColor rgbcolor = RgbColor(r, g, b);
            strip->SetPixelColor(i, rgbcolor);
        }
    }
}


/* update the while ledstrip. */
void updateWS2812()
{
    strip->Show();
}