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

#define AP_BUTTON             0
#define STATUS_LED_PIN        16
#define SERIALBAUD            115200
#define EFFECTPORT            1337
#define WEBSERVERPORT         80
#define EEPROMSIZE            1024
// set true if you want to connect to webpage,
// even if the esplight is in station mode.
#define SERVERTEST            false
// be able to upload new firmware remotely over wifi.
#define ENABLEOTA             false
// or you want serial debug output.
#define SERIALDEBUGOUTPUT     false
// set to true if you want the esplight to
// go into access point mode if it couldn't connect.
#define AP_AFTER_FAIL         false

// set initial board name and wifi settings.
String board_name = "EspLight-01";
String sta_ssid = "A_ssid_here";
String sta_pass = "A_password_here";

const uint8_t wifi_ip[4] = {192, 168, 1, 4};
const uint8_t wifi_subnet[4] = {255, 255, 255, 0};
const uint8_t wifi_gateway[4] = {192, 168, 1, 1};

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

Ticker statusTicker;

void statusUpdate()
{
  digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
}

/*
  stored are:
  board_name,
  sta ssid,
  sta pass,
  accesPin,
  stripselect,
  striplength
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
  //Serial.printf("\nread: %s \n", text);
  //Serial.printf("should be: %s \n", magicEepromWord.c_str());
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
  striplength
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
  Serial.println("checking for valid settings.");
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
    Serial.println("booting as access point!.");
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

void setupAP(bool silent)
{
  currentMode = AP_MODE;
  // setup the access point and print some debug if needed.
  WiFi.mode(WIFI_AP);
  WiFi.softAPdisconnect();
  delay(100);
  WiFi.softAP(board_name.c_str(), "");
  WiFi.softAPConfig(
    IPAddress(wifi_ip[0], wifi_ip[1], wifi_ip[2], wifi_ip[3]),
    IPAddress(wifi_gateway[0], wifi_gateway[1], wifi_gateway[2], wifi_gateway[3]),
    IPAddress(wifi_subnet[0], wifi_subnet[1], wifi_subnet[2], wifi_subnet[3])
  );
  if(!silent)
  {
    Serial.println();
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("access Point IP: ");
    Serial.println(myIP);
  }
  // indicate we are in AP_MODE with status led.
  statusTicker.attach(3.0, statusUpdate);
}

void setupSTA(bool silent)
{
  currentMode = STA_MODE;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
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
    // timeout after say 10 seconds.
    i++;
    if(i == 20)
    {
      if(!silent)
      {
        Serial.println("Unable to connect");
      }
      if(AP_AFTER_FAIL)
      {
        setupAP(silent);
      }
      break;
    }
  }
  // if we are connected indicate
  // that by flashing the status led slowly
  // also print status if not silenced
  if(WiFi.status() == WL_CONNECTED)
  {
    statusTicker.attach(1.0, statusUpdate);
    if(!silent)
    {
      Serial.println();
    }
    // print our current status for debugging.
    printWifiStatus();
  }
}

void wifiModeHandling()
{
  static uint8_t released = false;
  // check if button is pressed
  if(!digitalRead(AP_BUTTON))
  {
    // hold while down.
    while(!digitalRead(AP_BUTTON))yield();
    // if released then set released to true
    // and handle switching to accespoint.
    released = true;
  }
  else if(released)
  {
    released = false;
    Serial.println("mode is now AP_MODE (access point).");
    setupAP(false);
  }
}

void setupWifi(bool silent)
{
  WiFi.begin(board_name.c_str());
  if(currentMode == STA_MODE)
  {
    Serial.println("mode is now STA_MODE");
    setupSTA(silent);
  }
  else
  {
    Serial.println("mode is now AP_MODE");
    setupAP(silent);
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
  Serial.printf("Hostname set: %s\n", board_name.c_str());
  // scan for wifi once.
  available_aps = getAvailableNetworks();
}

void setup() {
  // setup the status led pin, and make it toggle fast,
  // indicating we are running the setup and still trying to connect.
  pinMode(STATUS_LED_PIN, OUTPUT);
  statusTicker.attach(0.05, statusUpdate);
  // setup serial uart communication.
  Serial.begin(SERIALBAUD);
  Serial.println();
  // prepare eeprom for use.
  EEPROM.begin(EEPROMSIZE);
  // settingsStore();
  // load stored settings.
  // load defaults if no valid settings..
  settingsLoad();

  // setup mode switching pin
  pinMode(AP_BUTTON, INPUT);

  // setup wifi with output.
  setupWifi(false);

  // setup strips for the first time (initialize some pointers and stuff.)
  setupStrips(striplen);
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
  else if(currentMode == STA_MODE)
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
        // also indicate with a status led by blinking slowly
        //statusTicker.attach(1.0, statusUpdate);
        setupOta();
        setupWebserver();
        setupEffectParse(EFFECTPORT);
      }
      delay(0);
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
