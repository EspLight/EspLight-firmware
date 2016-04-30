#include "stripcontrol.h"

void setupStrips(int striplen)
{
    // reset control pins.
    // sometimes they are on a special function.
    pinMode(WS2812_PIN, INPUT);

    if(striplen <= 1 || striplen >= 1000)
    {
        striplen = 1;
    }
    if(stripselect == ANALOGSTRIP)
    {
        setupAnalogStrip();
    }
    if(stripselect == WS2812)
    {
        // amount, pin
        setupWS2812(striplen, WS2812_PIN);
    }
    if(stripselect == WS2801)
    {
        // freq, amount
        setupWS2801(1e6, striplen);
    }
    delay(100);
}

void handleStrips()
{
    // check which effect is select.
    // and do pattern dependend on ledstrip.
    double brightfact = (stripcontrol.brightness+1)/100.0;
    int r = ((double)stripcontrol.varZero)*brightfact;
    int g = ((double)stripcontrol.varOne)*brightfact;
    int b = ((double)stripcontrol.varTwo)*brightfact;
    int speed = stripcontrol.varZero+1;
    if(stripcontrol.effect == RGBCOLORS)
    {
        if(stripselect == ANALOGSTRIP)
        {
            writeRgb(r, g, b);
        }
        else if(stripselect == WS2801)
        {
            setWS2801Strip(r, g, b);
            updateWS2801();
        }
        else if(stripselect == WS2812)
        {
            setWS2812Strip(r, g, b);
            updateWS2812();
        }
    }
    else if(stripcontrol.effect == FADING)
    {
        int brightness = stripcontrol.brightness+1;
        if(stripselect == ANALOGSTRIP)
        {
            fadeRgb(speed, brightness);
            delay(1);
        }
        else if(stripselect == WS2801)
        {
            fadeWS2801(speed, brightness);
            updateWS2801();
        }
        else if(stripselect == WS2812)
        {
            fadeWS2812(speed, brightness);
            updateWS2812();
            delay(1);
        }
    }
    // rainbow effect
    else if(stripcontrol.effect == DIGITALFADING)
    {
        int brightness = stripcontrol.brightness+1;
        if(stripselect == WS2812)
        {
            rainbowWS2812(speed, brightness);
            updateWS2812();
        }
        else if(stripselect == WS2801)
        {
            rainbowWS2801(speed, brightness);
            updateWS2801();
        }
    }
}

void debugPrintStripControl()
{
    char fmtstr[100];
    sprintf(fmtstr, 
        "\nDebug:\n"
        "pincode:     %d\n"
        "effect:      %d\n"
        "brightness:  %d\n"
        "var0:        %d\n"
        "var1:        %d\n"
        "var2:        %d\n"
        "changed:     %d\n",
        stripcontrol.pincode,
        stripcontrol.effect,
        stripcontrol.brightness,
        stripcontrol.varZero,
        stripcontrol.varOne,
        stripcontrol.varTwo,
        stripcontrol.changed
        );
    Serial.println(fmtstr);
}
