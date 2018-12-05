/*
ESP8266_ArtNetNode v2.0.0
Copyright (c) 2016, Matthew Tong
https://github.com/mtongnz/ESP8266_ArtNetNode_v2

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.
If not, see http://www.gnu.org/licenses/

Note:
This is a pre-release version of this software.  It is not yet ready for prime time and contains bugs (known and unknown).
Please submit any bugs or code changes so they can be included into the next release.

Prizes up for grabs:
I am giving away a few of my first batch of prototype PCBs.  They will be fully populated - valued at over $30 just for parts.
In order to recieve one, please complete one of the following tasks.  You can "win" multiple boards.
1 - Fix the WDT reset issue (https://github.com/mtongnz/ESP8266_ArtNetNode_v2/issues/41)
2 - Implement stored scenes function.  I want it to allow for static scenes or for chases to run.
3 - Most bug fixes, code improvements, feature additions & helpful submissions.
    eg. Fixing the flickering WS2812 (https://github.com/mtongnz/ESP8266_ArtNetNode_v2/issues/36)
        Adding other pixel strips (https://github.com/mtongnz/ESP8266_ArtNetNode_v2/issues/42)
        Creating new web UI theme (https://github.com/mtongnz/ESP8266_ArtNetNode_v2/issues/22)
        
These prizes will be based on the first person to submit a solution that I judge to be adequate.  My decision is final.
This competition will open to the general public a couple of weeks after the private code release to supporters.
*/

// Include extern files
#include "index.h"
#include "css.h"
#include "cssUploadPage.h"

// Include definitions
#include "globals.h"

// Include libraries
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <FS.h>
#include "store.h"
#include "espDMX.h"
#include "espArtNetRDM.h"
#include "ws2812Driver.h"
#include "wsFX.h"

extern "C" {
  #include "user_interface.h"
  extern struct rst_info resetInfo;
}

// Versions
#define FIRMWARE_VERSION "J.V2.1.0"
#define ART_FIRM_VERSION 0x0200   // Firmware given over Artnet (2 bytes)

/// INFO
// Wemos boards use 4M (3M SPIFFS) compiler option

#define WS2812_ALLOW_INT_SINGLE false
#define WS2812_ALLOW_INT_DOUBLE false

// Color definitions (numbers dont have an impact)
#define BLACK 0x00
#define RED 0x01
#define GREEN 0x02
#define ORANGE 0x03

uint8_t portA[5], portB[5];
uint8_t MAC_array[6];
uint8_t dmxInSeqID = 0;
uint8_t statusLedData[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
uint32_t statusTimer = 0;

espArtNetRDM artRDM;
ESP8266WebServer webServer(80);
DynamicJsonBuffer jsonBuffer;
ws2812Driver pixDriver;
File fsUploadFile;
bool statusLedsDim = true;
bool statusLedsOff = false;

pixPatterns pixFXA(0, &pixDriver);
pixPatterns pixFXB(1, &pixDriver);

// Types of Web Content
const char PROGMEM typeHTML[] = "text/html";
const char PROGMEM typeCSS[] = "text/css";

char wifiStatus[60] = "";
bool isHotspot = false;
uint32_t nextNodeReport = 0;
char nodeError[ARTNET_NODE_REPORT_LENGTH] = "";
bool nodeErrorShowing = 1;
uint32_t nodeErrorTimeout = 0;
bool pixDone = true;
bool newDmxIn = false;
bool doReboot = false;
byte* dataIn;

/// Logs a message without newline at the end. Only if Debug is enabled.
void Log(String message)
{
  #ifdef DEBUG_ENABLE
  Serial.print(message);
  #endif
}

/// Logs a message with newline at the end. Only if Debug is enabled.
void LogLn(String message)
{
  #ifdef DEBUG_ENABLE
  Serial.println(message);
  #endif
}

/// SETUP
void setup(void) 
{
  // Set direction to "input" to avoid boot garbage being sent out
  #ifndef DEBUG_ENABLE  // if debug is enabled the port A is not used
  pinMode(DMX_DIR_A, INPUT);
  digitalWrite(DMX_DIR_A, LOW);
  #endif

  pinMode(DMX_DIR_B, INPUT);
  digitalWrite(DMX_DIR_B, LOW);

  // Pin Modes of Status LEDs
  pinMode(DMX_ACT_LED_A, OUTPUT);
  pinMode(DMX_ACT_LED_B, OUTPUT);
  pinMode(STATUS_LED_S_R, OUTPUT);
  pinMode(STATUS_LED_S_G, OUTPUT);
  setStatusLed(RED);
  doStatusLedOutput();

  // Start Serial
  #ifdef DEBUG_ENABLE
  Serial.begin(115200);
  Serial.println(); // empty because of crap before serial starts
  #endif
  LogLn("Starting Setup");

  // Set WiFi Sleep Modes
  wifi_set_sleep_type(NONE_SLEEP_T);

  // Variable for Settings reset
  bool resetDefaults = false;

  // If the Reset Settings is activated
  #ifdef RESET_ENABLE
    pinMode(SETTINGS_RESET, INPUT);
    delay(5);
    
    // button pressed = low reading
    if (digitalRead(SETTINGS_RESET) == false) 
    {
      delay(50);  // request again after a longer delay
      if (digitalRead(SETTINGS_RESET) == false)
      {
        resetDefaults = true;
        LogLn("Settings Reset pressed: Settings will be reset.");
      }
    }
  #endif
  
  // Start EEPROM
  EEPROM.begin(512);

  // Start SPIFFS file system
  SPIFFS.begin();

  // Check if SPIFFS formatted
  if (SPIFFS.exists("/formatted.txt")) 
  {
    SPIFFS.format();
    
    File f = SPIFFS.open("/formatted.txt", "w");
    f.print("Formatted");
    f.close();
  }

  // Load our saved values or store defaults
  if (!resetDefaults)
  {
    eepromLoad();
    LogLn("Loaded Settings from EEPROM");
  }

  // Store our counters for resetting defaults
  if (resetInfo.reason != REASON_DEFAULT_RST && 
      resetInfo.reason != REASON_EXT_SYS_RST && 
      resetInfo.reason != REASON_SOFT_RESTART)
    deviceSettings.wdtCounter++;
  else
    deviceSettings.resetCounter++;

  // Store values
  eepromSave();

  // Start wifi
  wifiStart();

  // Start web server
  webStart();

  // Don't start our Artnet or DMX in firmware update mode or after multiple WDT resets
  if (!deviceSettings.doFirmwareUpdate && deviceSettings.wdtCounter <= 3) 
  {
    // We only allow 1 DMX input - and RDM can't run alongside DMX in
    if (deviceSettings.portAmode == TYPE_DMX_IN && deviceSettings.portBmode == TYPE_RDM_OUT)
      deviceSettings.portBmode = TYPE_DMX_OUT;
    
    // Setup Artnet Ports & Callbacks
    artStart();

    // Don't open any ports for a bit to let the ESP spill it's garbage to serial
    while (millis() < 3500)
      yield();
    
    // Port Setup
    portSetup();

  } 
  else  // if do firmware update
    deviceSettings.doFirmwareUpdate = false;

  delay(10);
}

void loop(void)
{
  // If the device lasts for 6 seconds, clear our reset timers
  if (deviceSettings.resetCounter != 0 && millis() > 6000) 
  {
    deviceSettings.resetCounter = 0;
    deviceSettings.wdtCounter = 0;
    eepromSave();
  }
  
  webServer.handleClient();
  
  // Get the node details and handle Artnet
  doNodeReport();
  artRDM.handler();
  
  yield();

  // DMX handlers
  #ifndef DEBUG_ENABLE  // if debug is enabled the port A is not used
  dmxA.handler();
  #endif
  dmxB.handler();


  // Do Pixel FX on port A
  #ifndef DEBUG_ENABLE  // if debug is enabled the port A is note used
  if (deviceSettings.portAmode == TYPE_WS2812 && deviceSettings.portApixMode != FX_MODE_PIXEL_MAP) {
    if (pixFXA.Update())
      pixDone = 0;
  }
  #endif

  // Do Pixel FX on port B
  if (deviceSettings.portBmode == TYPE_WS2812 && deviceSettings.portBpixMode != FX_MODE_PIXEL_MAP) {
    if (pixFXB.Update())
      pixDone = 0;
  }

  // Do pixel string output
  if (!pixDone)
    pixDone = pixDriver.show();
  
  // Handle received DMX
  if (newDmxIn) { 
    uint8_t g, p, n;
    
    newDmxIn = false;

    g = portA[0];
    p = portA[1];
    
    IPAddress bc = deviceSettings.dmxInBroadcast;
    artRDM.sendDMX(g, p, bc, dataIn, 512);
  }

  // Handle rebooting the system
  if (doReboot) {
    char c[ARTNET_NODE_REPORT_LENGTH] = "Device rebooting...";
    artRDM.setNodeReport(c, ARTNET_RC_POWER_OK);
    artRDM.artPollReply();
    
    // Ensure all web data is sent before we reboot
    uint32_t n = millis() + 1000;
    while (millis() < n)
      webServer.handleClient();

    ESP.restart();
  }
  

  // Output status to LEDs once per second
  if (statusTimer < millis()) {

    // Flash our main status LED
    if ((statusTimer % 2000) > 1000)
      setStatusLed(BLACK);
    else if (nodeError[0] != '\0')
      setStatusLed(RED);
    else
      setStatusLed(GREEN);
      

    doStatusLedOutput();
    statusTimer = millis() + 1000;
  }
}

void dmxHandle(uint8_t group, uint8_t port, uint16_t numChans, bool syncEnabled)
{
  if (portB[0] == group) {
    if (deviceSettings.portBmode == TYPE_WS2812) {

      setDmxLed(DMX_ACT_LED_B, true);   // flash Led => to High
      
      if (deviceSettings.portBpixMode == FX_MODE_PIXEL_MAP) {
        if (numChans > 510)
          numChans = 510;
        
        // Copy DMX data to the pixels buffer
        pixDriver.setBuffer(1, port * 510, artRDM.getDMX(group, port), numChans);
        
        // Output to pixel strip
        if (!syncEnabled)
          pixDone = false;

        return;

      // FX 12 mode
      } else if (port == portB[1]) {
        byte* a = artRDM.getDMX(group, port);
        uint16_t s = deviceSettings.portBpixFXstart - 1;
        
        pixFXB.Intensity = a[s + 0];
        pixFXB.setFX(a[s + 1]);
        pixFXB.setSpeed(a[s + 2]);
        pixFXB.Pos = a[s + 3];
        pixFXB.Size = a[s + 4];
        pixFXB.setColour1((a[s + 5] << 16) | (a[s + 6] << 8) | a[s + 7]);
        pixFXB.setColour2((a[s + 8] << 16) | (a[s + 9] << 8) | a[s + 10]);
        pixFXB.Size1 = a[s + 11];
        //pixFXB.Fade = a[s + 12];

        pixFXB.NewData = 1;
      }
    } else if (deviceSettings.portBmode != TYPE_DMX_IN && port == portB[1]) {
      dmxB.chanUpdate(numChans);
      setDmxLed(DMX_ACT_LED_B, false);   // flash Led => to High
    }
  }
  
  #ifndef DEBUG_ENABLE  // if debug is enabled the port A is not used
  else if (portA[0] == group)
  {
    if (deviceSettings.portAmode == TYPE_WS2812) {

      setDmxLed(DMX_ACT_LED_A, true);   // flash Led => to High
      
      if (deviceSettings.portApixMode == FX_MODE_PIXEL_MAP) {
        if (numChans > 510)
          numChans = 510;
        
        // Copy DMX data to the pixels buffer
        pixDriver.setBuffer(0, port * 510, artRDM.getDMX(group, port), numChans);
        
        // Output to pixel strip
        if (!syncEnabled)
          pixDone = false;

        return;

      // FX 12 Mode
      } else if (port == portA[1]) {
        byte* a = artRDM.getDMX(group, port);
        uint16_t s = deviceSettings.portApixFXstart - 1;
        
        pixFXA.Intensity = a[s + 0];
        pixFXA.setFX(a[s + 1]);
        pixFXA.setSpeed(a[s + 2]);
        pixFXA.Pos = a[s + 3];
        pixFXA.Size = a[s + 4];
        
        pixFXA.setColour1((a[s + 5] << 16) | (a[s + 6] << 8) | a[s + 7]);
        pixFXA.setColour2((a[s + 8] << 16) | (a[s + 9] << 8) | a[s + 10]);
        pixFXA.Size1 = a[s + 11];
        //pixFXA.Fade = a[s + 12];

        pixFXA.NewData = 1;
          
      }

    // DMX modes
    } 
    else if (deviceSettings.portAmode != TYPE_DMX_IN && port == portA[1]) 
    {
      dmxA.chanUpdate(numChans);
    }

    setDmxLed(DMX_ACT_LED_A, false);   // flash Led => to Low
  }
  #endif
}

void syncHandle() {
  #ifndef DEBUG_ENABLE  // if debug is enabled the port A is not used
  if (deviceSettings.portAmode == TYPE_WS2812) {
    rdmPause(1);
    pixDone = pixDriver.show();
    rdmPause(0);
  } else if (deviceSettings.portAmode != TYPE_DMX_IN)
    dmxA.unPause();
  #endif

  if (deviceSettings.portBmode == TYPE_WS2812) {
    rdmPause(1);
    pixDone = pixDriver.show();
    rdmPause(0);
  } else if (deviceSettings.portBmode != TYPE_DMX_IN)
    dmxB.unPause();
}

void ipHandle() {
  if (artRDM.getDHCP()) {
    deviceSettings.gateway = INADDR_NONE;
    
    deviceSettings.dhcpEnable = 1;
    doReboot = true;
    /*
    // Re-enable DHCP
    WiFi.begin(deviceSettings.wifiSSID, deviceSettings.wifiPass);

    // Wait for an IP
    while (WiFi.status() != WL_CONNECTED)
      yield();
    
    // Save settings to struct
    deviceSettings.ip = WiFi.localIP();
    deviceSettings.subnet = WiFi.subnetMask();
    deviceSettings.broadcast = {~deviceSettings.subnet[0] | (deviceSettings.ip[0] & deviceSettings.subnet[0]), ~deviceSettings.subnet[1] | (deviceSettings.ip[1] & deviceSettings.subnet[1]), ~deviceSettings.subnet[2] | (deviceSettings.ip[2] & deviceSettings.subnet[2]), ~deviceSettings.subnet[3] | (deviceSettings.ip[3] & deviceSettings.subnet[3])};

    // Pass IP to artRDM
    artRDM.setIP(deviceSettings.ip, deviceSettings.subnet);
    */
  
  } else {
    deviceSettings.ip = artRDM.getIP();
    deviceSettings.subnet = artRDM.getSubnetMask();
    deviceSettings.gateway = deviceSettings.ip;
    deviceSettings.gateway[3] = 1;
    deviceSettings.broadcast = {~deviceSettings.subnet[0] | (deviceSettings.ip[0] & deviceSettings.subnet[0]), ~deviceSettings.subnet[1] | (deviceSettings.ip[1] & deviceSettings.subnet[1]), ~deviceSettings.subnet[2] | (deviceSettings.ip[2] & deviceSettings.subnet[2]), ~deviceSettings.subnet[3] | (deviceSettings.ip[3] & deviceSettings.subnet[3])};
    deviceSettings.dhcpEnable = 0;
    
    doReboot = true;
    
    //WiFi.config(deviceSettings.ip,deviceSettings.ip,deviceSettings.ip,deviceSettings.subnet);
  }

  // Store everything to EEPROM
  eepromSave();
}

void addressHandle() {
  memcpy(&deviceSettings.nodeName, artRDM.getShortName(), ARTNET_SHORT_NAME_LENGTH);
  memcpy(&deviceSettings.longName, artRDM.getLongName(), ARTNET_LONG_NAME_LENGTH);

  #ifndef DEBUG_ENABLE  // if debug is enabled the port A is not used
  deviceSettings.portAnet = artRDM.getNet(portA[0]);
  deviceSettings.portAsub = artRDM.getSubNet(portA[0]);
  deviceSettings.portAuni[0] = artRDM.getUni(portA[0], portA[1]);
  deviceSettings.portAmerge = artRDM.getMerge(portA[0], portA[1]);

  if (artRDM.getE131(portA[0], portA[1]))
    deviceSettings.portAprot = PROT_ARTNET_SACN;
  else
    deviceSettings.portAprot = PROT_ARTNET;
  #endif
    
  deviceSettings.portBnet = artRDM.getNet(portB[0]);
  deviceSettings.portBsub = artRDM.getSubNet(portB[0]);
  deviceSettings.portBuni[0] = artRDM.getUni(portB[0], portB[1]);
  deviceSettings.portBmerge = artRDM.getMerge(portB[0], portB[1]);
  
  if (artRDM.getE131(portB[0], portB[1]))
    deviceSettings.portBprot = PROT_ARTNET_SACN;
  else
    deviceSettings.portBprot = PROT_ARTNET;
  
  // Store everything to EEPROM
  eepromSave();
}

void rdmHandle(uint8_t group, uint8_t port, rdm_data* c) 
{
  if (portB[0] == group && portB[1] == port)
    dmxB.rdmSendCommand(c);

  #ifndef DEBUG_ENABLE  // if debug is enabled the port A is not used
  else if (portA[0] == group && portA[1] == port)
    dmxA.rdmSendCommand(c);
  #endif
}

#ifndef DEBUG_ENABLE  // if debug is enabled the port A is not used
void rdmReceivedA(rdm_data* c)
{
  artRDM.rdmResponse(c, portA[0], portA[1]);
}

void sendTodA() {
  artRDM.artTODData(portA[0], portA[1], dmxA.todMan(), dmxA.todDev(), dmxA.todCount(), dmxA.todStatus());
}
#endif

void rdmReceivedB(rdm_data* c) {
  artRDM.rdmResponse(c, portB[0], portB[1]);
}

void sendTodB() {
  artRDM.artTODData(portB[0], portB[1], dmxB.todMan(), dmxB.todDev(), dmxB.todCount(), dmxB.todStatus());
}

void todRequest(uint8_t group, uint8_t port) 
{
  if (portB[0] == group && portB[1] == port)
    sendTodB();
  #ifndef DEBUG_ENABLE  // if debug is enabled the port A is not used
  else if (portA[0] == group && portA[1] == port)
    sendTodA();
  #endif
}

void todFlush(uint8_t group, uint8_t port) 
{
  if (portB[0] == group && portB[1] == port)
    dmxB.rdmDiscovery();
    
  #ifndef DEBUG_ENABLE  // if debug is enabled the port A is not used
  else if (portA[0] == group && portA[1] == port)
    dmxA.rdmDiscovery();
  #endif
}

void dmxIn(uint16_t num) {
  // Double buffer switch
  byte* tmp = dataIn;
  dataIn = dmxA.getChans();
  dmxA.setBuffer(tmp);
  
  newDmxIn = true;
}

void doStatusLedOutput() {
//STATUS LED!!!
/*
  uint8_t a[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
  
  if (!statusLedsOff) {
    if (statusLedsDim) {
      for (uint8_t x = 0; x < 9; x++)
        a[x] = statusLedData[x] & STATUS_DIM;
    } else {
      for (uint8_t x = 0; x < 9; x++)
        a[x] = statusLedData[x];
    }
  }

  #ifdef STATUS_LED_MODE_APA106
    pixDriver.doAPA106(&a[0], STATUS_LED_PIN, 9);
  #endif

  #ifdef STATUS_LED_MODE_WS2812
    pixDriver.doPixel(&a[0], STATUS_LED_PIN, 9);
  #endif

  // Tint LEDs red slightly - they'll be changed back before being displayed if no errors
  for (uint8_t x = 1; x < 9; x += 3)
    statusLedData[x] = 125;
     */
}


/// Status LED Function used for Status LED ///
void setStatusLed(uint32_t col) {
  switch (col)
  {
    case RED:
    {
      setRGled(true, false);
      break;
    }
    case GREEN:
    {
      setRGled(false, true);
      break;
    }
    case ORANGE:
    {
      setRGled(true, true);
      break;
    }
    case BLACK:
    {
      setRGled(false, false);
      break;
    }
  }
}

/// Status LED RG set Function used for Status LED ///
void setRGled(bool r, bool g)
{
  if(r)
  {
    analogWrite(STATUS_LED_S_R, STATUS_LED_BRIGHTNESS);
  }
  else
  {
    analogWrite(STATUS_LED_S_R, 0);
  }
  
  if(g)
  {
    analogWrite(STATUS_LED_S_G, STATUS_LED_BRIGHTNESS);
  }
  else
  {
    analogWrite(STATUS_LED_S_G, 0);
  }
}

/// Status LED Enable/Disable Function used for DMX LEDs ///
void setDmxLed(uint8_t pin, bool on)
{
  if(on)
  {
    analogWrite(pin, DMX_ACT_LED_BRIGHTNESS);
  }
  else
  {
    analogWrite(pin, 0);
  }
}

