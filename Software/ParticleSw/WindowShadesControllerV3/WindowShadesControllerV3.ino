// Window Shades V3
// Rob Dobson 2012-2016

// API
//   Set WiFi:       /IW/ssss/wwww/mmmm    - ssss = ssid, wwww = password, mmmm = WEP, WPA, WPA2 or NONE
//   WiFi clear:     /IW                   - clears stored SSID, etc
//   Set fixed IP:   /IP/iii/mmm/ggg/ddd   - iii = ip addr, mmm = subnet mask, ggg = gateway ip, ddd = dns server ip
//                   /IP/iii               - iii = ip addr, subnet mask = 255.255.255.0, gateway and dns ip = ip addr with last digit replaced with 1
//   IP use DHCP:    /IP/AUTO
//   Query IP/WiFi:  /IP
//   Shade Move:     /BLIND/#/cmd/duration    - # is the shade number,
//                                           cmd == "up", "down", "stop", "setuplimit", "setdownlimit", "resetmemory"
//                                           duration is "pulse", "on", "off"
//   Wipe config:    /WC/1234             - wipe config EEPROM
//   Query status:   /Q
//   Set shades:     /SHADECFG/#/name1/name2/name3/name4/name5/name6
//                                        - # = number of shades, name1..6 = shade name
//                                        - responds with same info as Query Status
//
//   Add Notify:     /NO/iii:ppp          - add a request for status notification on a specific ip address and port e.g. 192.168.0.76:25366
//   Wipe config:    /WC/1234             - wipe config EEPROM

// EEPROM Storage
// SSID: 31 bytes
// WiFiPW: 31 bytes
// IP and port str: 31 bytes

//#define DEBUG_CLEAR_EEPROM 1

#define RD_DEBUG_LEVEL 2
#define RD_DEBUG_FNAME "WindowShadesControllerV3"
#include "RdDebugLevel.h"

#include "WiFiConn.h"
#include "RdWebServer.h"
#include "ParticleCloud.h"
#include "GenResources.h"
#include "ConfigDb.h"
#include "WindowShades.h"
#include "Utils.h"
#include "NotifyMgr.h"

/*
 * SYSTEM_MODE:
 *     - AUTOMATIC: Automatically try to connect to Wi-Fi and the Particle Cloud and handle the cloud messages.
 *     - SEMI_AUTOMATIC: Manually connect to Wi-Fi and the Particle Cloud, but automatically handle the cloud messages.
 *     - MANUAL: Manually connect to Wi-Fi and the Particle Cloud and handle the cloud messages.
 *
 * SYSTEM_MODE(AUTOMATIC) does not need to be called, because it is the default state.
 * However the user can invoke this method to make the mode explicit.
 * Learn more about system modes: https://docs.particle.io/reference/firmware/photon/#system-modes .
 */
//#if defined(ARDUINO)
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(MANUAL);
//#endif

// Hardware definitions
const int HC595_SER = D0;              // 75HC595 pins
const int HC595_SCK = D1;              //
const int HC595_RCK = D2;              //
const int LED_OP = D3;
const int LED_ACT = D4;
const int SENSE_A0 = A0;
const int SENSE_A1 = A1;
const int SENSE_A2 = A2;

// Network SSID
String wifiSSID = "";
// Network password
String wifiPassword = "";
// Wifi connect method
String wifiConnMethod = "WPA2";
// Wifi IP Address info
String wifiIPAddr = "AUTO";
String wifiSubnetMask = "";
String wifiGatewayIP = "";
String wifiDNSIP = "";

// Web server port
int webServerPort = 80;

// Layout of EEPROM
const int EEPROM_generalConfig_POS = 0;

// Config is split into records
const int CONFIG_RECIDX_FOR_INTERNET = 0;
const int CONFIG_RECIDX_FOR_SHADES = 1;

// Particle Cloud
ParticleCloud* pParticleCloud = NULL;

// WiFi connection and server
WiFiConn* pWiFiConn = NULL;
RdWebServer* pWebServer = NULL;

// Config and user DBs
ConfigDb* pConfigDb = NULL;

// Doors
WindowShades* pWindowShades = NULL;

// Notifications
NotifyMgr* pNotifyMgr = NULL;

// Rest API Implementations
const int MAX_REST_API_RETURN_LEN = 1000;
static char restAPIHelpersBuffer[MAX_REST_API_RETURN_LEN];
char* setResultStr(bool rslt)
{
    sprintf(restAPIHelpersBuffer, "{ \"rslt\": \"%s\" }", rslt? "ok" : "fail");
    return restAPIHelpersBuffer;
}
#include "RestAPIWiFiAndIP.h"
#include "RestAPIHelpers.h"
#include "RestAPIUtils.h"

void startWiFi()
{
  // Get internet config values
  String configSSID, configPassword, configConnMethod, configIPAddr;
  pConfigDb->getRecValByName(CONFIG_RECIDX_FOR_INTERNET, "SSID", configSSID);
  if (configSSID.length() != 0)
    wifiSSID = configSSID;
  pConfigDb->getRecValByName(CONFIG_RECIDX_FOR_INTERNET, "PW", configPassword);
  if (configPassword.length() != 0)
    wifiPassword = configPassword;
  pConfigDb->getRecValByName(CONFIG_RECIDX_FOR_INTERNET, "METH", configConnMethod);
  if (configConnMethod.length() > 0)
    wifiConnMethod = configConnMethod;
  pConfigDb->getRecValByName(CONFIG_RECIDX_FOR_INTERNET, "IP", configIPAddr);
  if (configIPAddr.length() > 0)
    wifiIPAddr = configIPAddr;
  Serial.print("Got IP Addr from config");
  Serial.print(configIPAddr);
  Serial.print(", wifiIP: ");
  Serial.println(wifiIPAddr);
  pConfigDb->getRecValByName(CONFIG_RECIDX_FOR_INTERNET, "MASK", wifiSubnetMask);
  pConfigDb->getRecValByName(CONFIG_RECIDX_FOR_INTERNET, "GATE", wifiGatewayIP);
  pConfigDb->getRecValByName(CONFIG_RECIDX_FOR_INTERNET, "DNS", wifiDNSIP);
  if (pWiFiConn)
  {
      pWiFiConn->setConnectParams(wifiSSID, wifiPassword, wifiConnMethod, wifiIPAddr, wifiSubnetMask, wifiGatewayIP, wifiDNSIP);
      pWiFiConn->initialConnect();
  }
}

void setup()
{
  // Construct Window shades first - so outputs are reset
  pWindowShades = new WindowShades(HC595_SER, HC595_SCK, HC595_RCK);

  // initialize serial communication
  Serial.begin(115200);

  // Short delay before message
  delay(1000);
  Serial.println("WindowShades V3.0 2016Oct03");

  #ifdef DEBUG_CLEAR_EEPROM
    EEPROM.clear();
    Serial.println("EEPROM Cleared");
  #endif

  // EEPROM debug
  #ifdef SHOW_EEPROM_DEBUG
    size_t length = EEPROM.length();
    Serial.printlnf("EEPROM has %d bytes available", length);
  #endif

  // Construct config DB
  pConfigDb = new ConfigDb(EEPROM_generalConfig_POS, 1500);

  // Particle Cloud
  pParticleCloud = new ParticleCloud(handleReceivedApiStr, restHelper_QueryStatus);
  pParticleCloud->RegisterVariables();

  // Construct server and WiFi
  pWiFiConn = new WiFiConn();
  pWebServer = new RdWebServer(pWiFiConn);

  // Notifications handler
  pNotifyMgr = new NotifyMgr(restHelper_QueryStatus);

  // Start WiFi
  startWiFi();

  // Configure web server
  if (pWebServer)
  {
    // Add resources to web server
    pWebServer->addStaticResources(genResources, genResourcesCount);

    // Add network management REST API commands to web server
    pWebServer->addCommand("IW", RdWebServerCmdDef::CMD_CALLBACK, restAPI_Wifi);
    pWebServer->addCommand("IP", RdWebServerCmdDef::CMD_CALLBACK, restAPI_NetworkIP);

    // Query
    pWebServer->addCommand("Q", RdWebServerCmdDef::CMD_CALLBACK, restAPI_QueryStatus);

    // Add window shades REST API commands to web server
    pWebServer->addCommand("BLIND", RdWebServerCmdDef::CMD_CALLBACK, restAPI_ShadesControl);
    pWebServer->addCommand("SHADECFG", RdWebServerCmdDef::CMD_CALLBACK, restAPI_ShadesConfig);

    // Add notifications
    pWebServer->addCommand("NO", RdWebServerCmdDef::CMD_CALLBACK, restAPI_RequestNotifications);

    // Wipe config
    pWebServer->addCommand("WC", RdWebServerCmdDef::CMD_CALLBACK, restAPI_WipeConfig);

    // Start the web server
    pWebServer->start(webServerPort);
  }

  // Status LEDs
  pinMode(LED_OP, OUTPUT);
  pinMode(LED_ACT, OUTPUT);
}

// Timing of the loop - used to determine if blocking/slow processes are delaying the loop iteration
const int loopTimeAvgWinLen = 50;
int loopTimeAvgWin[loopTimeAvgWinLen];
int loopTimeAvgWinHead = 0;
int loopTimeAvgWinCount = 0;
unsigned long loopTimeAvgWinSum = 0;
unsigned long lastLoopStartMicros = 0;
unsigned long lastDebugLoopTime = 0;

void loop()
{

  // Service the particle cloud
  if (pParticleCloud)
    pParticleCloud->Service();

  // Monitor how long it takes to go around loop
  if (lastLoopStartMicros != 0)
  {
    unsigned long loopTime = micros() - lastLoopStartMicros;
    if (loopTime > 0)
    {
      if (loopTimeAvgWinCount == loopTimeAvgWinLen)
      {
        int oldVal = loopTimeAvgWin[loopTimeAvgWinHead];
        loopTimeAvgWinSum -= oldVal;
      }
      loopTimeAvgWin[loopTimeAvgWinHead++] = loopTime;
      if (loopTimeAvgWinHead >= loopTimeAvgWinLen)
        loopTimeAvgWinHead = 0;
      if (loopTimeAvgWinCount < loopTimeAvgWinLen)
        loopTimeAvgWinCount++;
      loopTimeAvgWinSum += loopTime;
    }
  }
  lastLoopStartMicros = micros();
  if (millis() > lastDebugLoopTime + 10000)
  {
    if (loopTimeAvgWinLen > 0)
    {
      RD_DBG("Avg loop time %0.6f (val %lu)", 1.0 * loopTimeAvgWinSum / loopTimeAvgWinLen, lastLoopStartMicros);
    }
    else
    {
      RD_DBG("No avg loop time yet");
    }
    lastDebugLoopTime = millis();
  }

  // Service the WiFi connection
  if (pWiFiConn)
    pWiFiConn->service();

  // Service the web server
  if (pWebServer)
  {
    pWebServer->service();
    int connState = pWebServer->connState();
    digitalWrite(LED_ACT, connState == RdWebServer::WebServerState::WEB_SERVER_CONNECTED_BUT_NO_CLIENT);
    digitalWrite(LED_OP, connState == RdWebServer::WebServerState::WEB_SERVER_HAS_CLIENT);
  }

  // Service the window shades
  if (pWindowShades)
    pWindowShades->service();

  // Service notifications
  if (pNotifyMgr)
    pNotifyMgr->service();
}
