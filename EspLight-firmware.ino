/*
  Author: Duality / Robert

  This is Firmware for controlling ledstrips with a esp8266.
  It includes a way of setting which strip is connected, (webinterface)
  and how long that ledstrip is.
  It also includes a Way of finding the device through
  broadcastig a udp packet. (esplightcontroller app)
  It also includes a way for controlling which effect is selected.
*/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <Ticker.h>

#include <otaupload.h>

#include "stripcontrol.h"
#include "html.h"
#include "effectParse.h"

#define AP_BUTTON           0
#define STATUS_LED_PIN      16
#define SERIALBAUD          115200
#define EFFECTPORT          1337
#define WIFI_CHANNEL        4
#define WEBSERVERPORT       80
#define EEPROMSIZE          1024
#define SERVERTEST          false
#define ENABLEOTA           false
// enable all Serial printing.
#define SERIALDEBUGPRINTING true
// sets Serial.setDebugOutput()
#define ENABLESERIALDEBUG   true

// set initial board name and wifi settings.
String board_name = "EspLight01";
String sta_ssid = "esplight_ssid";
String sta_pass = "esplight_password";

// select an initial mode.
enum {STA_MODE, AP_MODE};
int currentMode = STA_MODE;

// set an initial pincode.
int accessPin = 1234;

String magicEepromWord = "deadbeaf";
uint8_t magicWorldLen;

// create a stripcontrol structure and clear it.
stripcontrol_t stripcontrol = {
  .pincode = 0,
  .effect = 0,
  .brightness = 0,
  .varZero = 0,
  .varOne = 0,
  .varTwo = 0,
  .changed = false
};

String available_aps;

// select intial ledstrip
int stripselect = ANALOGSTRIP;
// select an initial length.
int striplen = 1;

// setup http server
ESP8266WebServer server(WEBSERVERPORT);

// setup ticker and status blink function.
Ticker statusBlinker;

// types that define speed intervals that indicate status on led.
typedef struct 
{
  int connecting;
  int connected;
  int apmode;
  int error;
} board_state_t;

board_state_t board_state = 
{
  .connecting = 85,
  .connected = 1000,
  .apmode = 3000,
  .error = -1
};

// the actuall function that sets the state of the led.
void statusBlink()
{
  static uint8_t state;
  digitalWrite(STATUS_LED_PIN, state);
  state = !state;
}

// function to set a state.
void setStatus(int state)
{
  if(state == board_state.error)
  {
    statusBlinker.detach();
    digitalWrite(STATUS_LED_PIN, LOW);
  }
  else
  {
    statusBlinker.attach_ms(state, statusBlink);
  }
}

// setup the ledstate.
void setupStatusLed()
{
  pinMode(STATUS_LED_PIN, OUTPUT);
  setStatus(board_state.connecting);
}

Ticker heapPrinter;

void heapPrint()
{
  Serial.printf("heap: %d \n", ESP.getFreeHeap());
}

void setupHeapPrint()
{
  heapPrinter.attach(10, heapPrint);
}

// stores a string into eeprom plus nullterminator.
void storeString(String string, int& addr)
{
  // get a reference to the original c string.
  const char *str = string.c_str();
  // get the length of that string.
  int str_len = strlen(str);
  int i;
  // loop over the length of the string.
  // and store the bytes. and zero terminator.
  for(i = 0; i <= str_len; i++)
  {
    // write the bytes into the eeprom (flash)
    EEPROM.write(addr+i, str[i]);
    delay(0);
  }
  addr += i;
}

void storeInt(int value, int& addr)
{
  char fmtstr[100];
  int i;
  for(i = 0; i < sizeof(value); i++)
  {
    EEPROM.write(addr+i, (value>>(i*8)&0xff));
    delay(0);
  }
  addr += i;
}

String loadString(int &addr)
{
  String text = "";
  char read = EEPROM.read(addr);
  /* Basicly how you read any string in C. */
  while(read != '\0')
  {
    text += read;
    addr++;
    read = EEPROM.read(addr);
    delay(0);
  }
  addr++; //acount for zero terminator.
  return text;
}

// checks for the magic string in eeprom indicating valid settings.
bool magicStrPresent(int &addr)
{
  char text[] = "deadbeaf";
  uint8_t magicWordLen = magicEepromWord.length();
  for(int i = 0; i < magicWordLen; i++)
  {
    text[i] = EEPROM.read(addr);
    addr++;
    delay(0);
  }
  addr++;
  //acount for zero terminator
  Serial.printf("\nread: %s \n", text);
  Serial.printf("should be: %s \n", magicEepromWord.c_str());
  return (String(text) == magicEepromWord);
}

int loadInt(int &addr)
{
  int value = 0;
  int i;
  for(i = 0; i < 0x04; i++)
  {
    char byte = EEPROM.read(addr+i);
    value |= (byte << (8*i));
  }
  addr += i;
  return value;
}

void settingsStore()
{
  /*
  stored are:
  board_name,
  sta ssid,
  sta pass,
  accesPin,
  stripselect,
  currentMode
  */
  int eeAddr = 0;
  Serial.println("storing: ");
  storeString(magicEepromWord, eeAddr);
  Serial.println("magic word:");
  Serial.println(magicEepromWord);
  storeString(board_name, eeAddr);
  Serial.println("board name:");
  Serial.println(board_name);
  storeString(sta_ssid, eeAddr);
  Serial.println("ssid: ");
  Serial.println(sta_ssid);
  storeString(sta_pass, eeAddr);
  Serial.println("pass: ");
  Serial.println(sta_pass);
  storeInt(accessPin, eeAddr);
  Serial.println("accesspin");
  Serial.println(accessPin);
  storeInt(stripselect, eeAddr);
  Serial.println("strip select: ");
  Serial.println(stripselect);
  storeInt(striplen, eeAddr);
  Serial.println("striplen: ");
  Serial.println(striplen);
  EEPROM.commit();
}

void settingsLoad()
{
  int eeAddr = 0;

  bool isValid = magicStrPresent(eeAddr);
  // check if settings are valid;
  Serial.print("settings are valid: ");
  Serial.println(isValid ? "true" : "false");

  if(isValid)
  {
    // valid settings found and load them.
    board_name = loadString(eeAddr);
    sta_ssid = loadString(eeAddr);
    sta_pass = loadString(eeAddr);
    accessPin = loadInt(eeAddr);
    stripselect = loadInt(eeAddr);
    striplen = loadInt(eeAddr);
  }
  else if(!isValid)
  {
    // no valid settings found.
    // store valid settings.
    Serial.println("invalid settings found loading defaults.");
    settingsStore();
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void setupAP()
{
  currentMode = AP_MODE;
  WiFi.mode(WIFI_OFF);
  delay(0);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(board_name.c_str());
  Serial.println();
  IPAddress myIP = WiFi.softAPIP();
  Serial.println(myIP);
  setStatus(board_state.apmode);
}

void setupSTA(bool silent)
{
  currentMode = STA_MODE;
  WiFi.mode(WIFI_STA);
  // WiFi.disconnect();
  WiFi.begin(sta_ssid.c_str(), sta_pass.c_str(), WIFI_CHANNEL);
  if(!silent)
  Serial.println();
  // timeout variable.
  int i = 0;
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    if(!silent)
    Serial.print(".");
    // if button pressed switch mode.
    wifiModeHandling();
    // keep a timeout timer.
    i++;
    if(i == 20)
    {
      if(!silent)
      Serial.println("Unable to connect");
      return;
    }
  }
  setStatus(board_state.connected);
  if(!silent)
  {
    Serial.println();
    printWifiStatus();
  }
}

void wifiModeHandling()
{
  if(!digitalRead(AP_BUTTON))
  {
    // switch modes as needed.
    if(currentMode == STA_MODE)
    {
      Serial.println("mode is now AP_MODE");
      setupAP();
    }
    /* actually never gets here. but does if we allowed it to switch back.*/
    // else
    // {
    //   Serial.println("mode is now STA_MODE");
    //   // list to output that we are connecting
    //   setupSTA(false);
    // }
    while(!digitalRead(AP_BUTTON)) delay(50);
  }
}

void setupWifi(bool silent)
{
  if(currentMode == STA_MODE)
  {
    Serial.println("mode is now STA_MODE");
    setupSTA(silent);
  }
  else
  {
    Serial.println("mode is now AP_MODE");
    setupAP();
  }
}

void setupWebserver()
{
  // attach directories and file handlers.
  server.on("/", handleRoot);
  server.on("/ledsettings", handleLedSettings);
  server.on("/wifisettings", handleWiFiSettings);
  server.on("/css", handleCss);
  // start the server
  server.begin();
  Serial.println("done setting up server");
  // request a hostname
  WiFi.hostname(board_name);
  Serial.printf("Hostname set: %s\n", board_name.c_str());
  // scan for wifi once.
  available_aps = getAvailableNetworks();
}

void setup() {
  // setup serial com.
  if(SERIALDEBUGPRINTING)
  {
    Serial.begin(SERIALBAUD);
    Serial.setDebugOutput(ENABLESERIALDEBUG);
    Serial.println();
  }
  // prepare eeprom for use.
  EEPROM.begin(EEPROMSIZE);
  // settingsStore();
  // load stored settings.
  settingsLoad();

  // setup led status
  setupStatusLed();
  // setup mode switching pin
  pinMode(AP_BUTTON, INPUT_PULLUP);

  // setup strips for the first time (initialize some pointers and stuff.)
  // and also turn off the leds
  setupStrips(striplen);

  // setup wifi with output.
  setupWifi(false);
  // enable OTA
  Serial.setDebugOutput(ENABLESERIALDEBUG);
  setupOta();

  Serial.println("done setting up pins, and WifiMode.");

  /* Setup server side things.*/
  setupWebserver();
  // setup the effect parser.
  setupEffectParse(EFFECTPORT);

  // setup heap printing
  setupHeapPrint();
}

void loop() {
  if((currentMode == AP_MODE) || SERVERTEST)
  {
    // only serve pages in Access point mode.
    server.handleClient();
  }
  if(currentMode == STA_MODE)
  {
    // check if we are still connected.
    if(WiFi.status() != WL_CONNECTED)
    {
      // don't list anything to the serial output.
      // but still try to connect.
      setupSTA(true);
      if(WiFi.status() == WL_CONNECTED)
      {
        // if reconnected restart anything that uses a port.
        setupOta();
        setupWebserver();
        setupEffectParse(EFFECTPORT);
      }
    }
    // handle ledstrip animations.
    handleStrips();
    // handle control over effects.
    handleEffectUpdate();
    // handle switching to AP_MODE
    wifiModeHandling();
  }
  if(ENABLEOTA)
  {
    // allow uploading over OTA <dev>
    handleSketchUpdate();
  }
}
