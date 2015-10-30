#include "ws2801.h"

// hold array of colors 
// r[], g[], b[]
static uint8_t *buffer[3] = {NULL, NULL, NULL};

// length of ledstrip in chips
static int ws2801_striplen;


// timing variables.
static unsigned long ccurrent = 0;
static unsigned long cprevious = 0;
static int cinterval = 10;

// pointer to generated colors.
static uint8_t *colors = NULL;

/* setup basics like spi, allocate space for the pixels.*/
void setupWS2801(int freq, int len)
{
    SPI.begin();
    SPI.setFrequency(1e6);
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
    ws2801_striplen = len;
    colors = colorinc();
    // allocate space if run first time else free and allocate.
    if(buffer[0] == NULL && buffer[1] == NULL && buffer[2] == NULL)
    {
        // allocate space for buffer
        buffer[0] = (uint8_t *)malloc(sizeof(uint8_t)*len);
        buffer[1] = (uint8_t *)malloc(sizeof(uint8_t)*len);
        buffer[2] = (uint8_t *)malloc(sizeof(uint8_t)*len);
    }
    else
    {
        free(buffer[0]);
        free(buffer[1]);
        free(buffer[2]);
        // allocate space for buffer
        buffer[0] = (uint8_t *)malloc(sizeof(uint8_t)*len);
        buffer[1] = (uint8_t *)malloc(sizeof(uint8_t)*len);
        buffer[2] = (uint8_t *)malloc(sizeof(uint8_t)*len);
    }
}

/* set a certain pixel to certain (r, g, b) values.*/
void setWS2801Pixel(int pos, int r, int g, int b)
{
    buffer[RED][pos] = r;
    buffer[GREEN][pos] = g;
    buffer[BLUE][pos] = b;
}

/* set a whole ledstrip to a single color.*/
void setWS2801Strip(int r, int g, int b)
{
    for(int i = 0; i < ws2801_striplen; i++)
    {
        setWS2801Pixel(i, r, g, b);
    }
}

/* effect that fades all the leds from one color to another.*/
void fadeWS2801(int speed, int brightness)
{
    ccurrent = millis();
    if((ccurrent - cprevious) >= cinterval)
    {
        cprevious = ccurrent;
        colors = colorinc();
    }

    cinterval = speed+1;
    float brightnessFactor = (float)(((float)brightness)/100);
    int r = colors[RED] * brightnessFactor;
    int g = colors[GREEN] * brightnessFactor;
    int b = colors[BLUE] * brightnessFactor;

    setWS2801Strip(r, g, b);
}
/* 
    fade all the pixels individually from one color to the next.
    creating a rainbow like effect.
*/
void rainbowWS2801(int speed, int brightness)
{
    cinterval = speed + 1;
    ccurrent = millis();
    if((ccurrent - cprevious) >= cinterval)
    {
        cprevious = ccurrent;
        static int range = 0xff*3;
        uint8_t buffer[3][ws2801_striplen];
        int i, s;
        colors = colorinc();
        for(i = 0; i < ws2801_striplen; i++)
        {
            for(s = 0; s < range/ws2801_striplen; s++)
                colors = colorinc();
            float brightnessFactor = (float)(((float)brightness) / 100);
            int r = colors[RED] * brightnessFactor;
            int g = colors[GREEN] * brightnessFactor;
            int b = colors[BLUE] * brightnessFactor;
            setWS2801Pixel(i, r, g, b);
        }
    }
}

/* update the whole ledstrip with it's colors.*/
void updateWS2801()
{
    for(int i = 0; i < ws2801_striplen; i++)
    {
        SPI.transfer(buffer[RED][i]);
        SPI.transfer(buffer[GREEN][i]);
        SPI.transfer(buffer[BLUE][i]);
    }
    delay(1);
}