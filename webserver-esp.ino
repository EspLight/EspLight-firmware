/*
  Author: Duality / Robert

  This is Firmware for controlling ledstrips.
  It includes a way of setting which strip is connected,
  and how long that ledstrip is.
  It also includes a Way of finding the device through
  broadcastig a udp packet.
  It also includes a way for controlling which effect is selected.
*/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>

#include <otaupload.h>

#include "stripcontrol.h"
#include "html.h"
#include "effectParse.h"

#define AP_BUTTON         0// 12
#define SERIALBAUD        115200
#define EFFECTPORT        1337
#define WEBSERVERPORT     80
#define EEPROMSIZE        1024
#define SERVERTEST        false
#define ENABLEOTA         false
#define SERIALDEBUGOUTPUT false

// set initial board name and wifi settings.
String board_name = "EspLight-01";
String sta_ssid = "A_ssid_here";
String sta_pass = "A_password_here";

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

/*
  stored are:
  board_name,
  sta ssid,
  sta pass,
  accesPin,
  stripselect,
  currentMode
  */

ESP8266WebServer server(WEBSERVERPORT);

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
  storeString(magicEepromWord, eeAddr);
  storeString(board_name, eeAddr);
  storeString(sta_ssid, eeAddr);
  storeString(sta_pass, eeAddr);
  storeInt(accessPin, eeAddr);
  storeInt(stripselect, eeAddr);
  storeInt(striplen, eeAddr);
  EEPROM.commit();
}

void settingsLoad()
{
  int eeAddr = 0;

  bool isValid = magicStrPresent(eeAddr);
  // check if settings are valid;
  // Serial.print("settings are valid: ");
  // Serial.println(isValid ? "true" : "false");

  if(isValid)
  {
    Serial.println("valid settings found and loading.");
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
  WiFi.mode(WIFI_AP);
  WiFi.softAP(board_name.c_str());
  Serial.println();
  IPAddress myIP = WiFi.softAPIP();
  Serial.println(myIP);
}

void setupSTA(bool silent)
{
  currentMode = STA_MODE;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(sta_ssid.c_str(), sta_pass.c_str());
  if(!silent)
  Serial.println();
  // timeout variable.
  int i = 0;
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    if(!silent)
    Serial.write('.');
    // if button pressed switch mode.
    wifiModeHandling();
    // keep a timeout timer.
    i++;
    if(i == 10)
    {
      if(!silent)
      Serial.println("Unable to connect");
      return;
    }
  }
  if(!silent)
  Serial.println();
  printWifiStatus();
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
  Serial.begin(SERIALBAUD);
  Serial.println();
  // prepare eeprom for use.
  EEPROM.begin(EEPROMSIZE);
  // settingsStore();
  // load stored settings.
  settingsLoad();

  // setup mode switching pin
  pinMode(AP_BUTTON, INPUT_PULLUP);

  // setup strips for the first time (initialize some pointers and stuff.)
  setupStrips(striplen);

  // setup wifi with output.
  setupWifi(false);
  // enable OTA
  Serial.setDebugOutput(SERIALDEBUGOUTPUT);
  setupOta();

  Serial.println("done setting up pins, and WifiMode.");

  /* Setup server side things.*/
  setupWebserver();
  // setup the effect parser.
  setupEffectParse(EFFECTPORT);
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
