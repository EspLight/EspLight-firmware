#ifndef WS2801_H
#define WS2801_H

#include <SPI.h>
#include "stripcontrol.h"
#include "fadeColor.h"
#include "rainbow.h"

void setupWS2801(int, int);
void setWS2801Pixel(int, int, int, int);
void setWS2801Strip(int, int, int);
void fadeWS2801(int, int);
void rainbowWS2801(int, int);
void updateWS2801();

#endif