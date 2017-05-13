// Window Shades V3.1
// Rob Dobson 2012-2017

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

//#define DEBUG_CLEAR_EEPROM 1

// Particle Threading and cloud connection
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(AUTOMATIC);

// Logging
SerialLogHandler logHandler(LOG_LEVEL_TRACE);

// Utils
#include "Utils.h"
Utils _utils;

// Config
#include "ConfigEEPROM.h"
ConfigEEPROM configEEPROM;
static const char* EEPROM_CONFIG_LOCATION_STR =
    "{\"base\": 0, \"maxLen\": 1000}";

// ParticleCloud support
#include "ParticleCloud.h"
ParticleCloud* pParticleCloud = NULL;

// API Endpoints
#include "RestAPIEndpoints.h"
RestAPIEndpoints restAPIEndpoints;

// Web server
#include "RdWebServer.h"
const int webServerPort = 80;
RdWebServer* pWebServer = NULL;
#include "GenResources.h"

// Notifications
#include "NotifyMgr.h"
NotifyMgr* pNotifyMgr = NULL;

// Window shades
#include "WindowShades.h"
WindowShades* pWindowShades = NULL;
const int HC595_SER = D0;              // 75HC595 pins
const int HC595_SCK = D1;              //
const int HC595_RCK = D2;              //
const int LED_OP = D3;
const int LED_ACT = D4;
const int SENSE_A0 = A0;
const int SENSE_A1 = A1;
const int SENSE_A2 = A2;

// API Support (for web, etc)
#include "RestAPIUtils.h"
#include "RestAPIHelpers.h"
#include "RestAPIWiFiAndIP.h"


void setupRestAPIEndpoints()
{

    // Add network management REST API commands
    restAPIEndpoints.addEndpoint("IW", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_Wifi);
    restAPIEndpoints.addEndpoint("IP", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_NetworkIP);

    // Query
    restAPIEndpoints.addEndpoint("Q", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_QueryStatus);

    // Add window shades REST API commands to web server
    restAPIEndpoints.addEndpoint("BLIND", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_ShadesControl);
    restAPIEndpoints.addEndpoint("SHADECFG", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_ShadesConfig);

    // Add notifications
    restAPIEndpoints.addEndpoint("NO", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_RequestNotifications);

    // Reset device
    restAPIEndpoints.addEndpoint("RESET", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_Reset);

    // Wipe config
    restAPIEndpoints.addEndpoint("WIPEALL", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_WipeConfig);
}

void setup()
{
    // Construct Window shades first - so outputs are reset
    pWindowShades = new WindowShades(HC595_SER, HC595_SCK, HC595_RCK);

    // Short delay before message
    delay(5000);
    Log.info("WindowShades V3.1 2017/05/12");

    #ifdef DEBUG_CLEAR_EEPROM
    EEPROM.clear();
    Log.info("EEPROM Cleared");
    #endif

    // EEPROM debug
    size_t length = EEPROM.length();
    Log.info("EEPROM has %d bytes available", length);

    // Initialise the config manager
    configEEPROM.setConfigLocation(EEPROM_CONFIG_LOCATION_STR);

    // Particle Cloud
    pParticleCloud = new ParticleCloud(handleReceivedApiStr, restHelper_QueryStatus);
    pParticleCloud->RegisterVariables();

    // Construct web server
    pWebServer = new RdWebServer();

    // Setup REST API endpoints
    setupRestAPIEndpoints();

    // Configure web server
    if (pWebServer)
    {
        // Add resources to web server
        pWebServer->addStaticResources(genResources, genResourcesCount);
        pWebServer->addRestAPIEndpoints(&restAPIEndpoints);
        // Start the web server
        pWebServer->start(webServerPort);
    }

    // Notifications handler
    pNotifyMgr = new NotifyMgr();
    /*pNotifyMgr->addNotifyType(1, restHelper_ReportHealth,
                        restHelper_ReportHealthHash, NotifyMgr::NOTIFY_POST);*/

    // Status LEDs
    pinMode(LED_OP, OUTPUT);
    pinMode(LED_ACT, OUTPUT);

}

// Timing of the loop - used to determine if blocking/slow processes are delaying the loop iteration
const int loopTimeAvgWinLen = 50;
int loopTimeAvgWinUs[loopTimeAvgWinLen];
int loopTimeAvgWinHead = 0;
int loopTimeAvgWinCount = 0;
unsigned long loopTimeSumUs = 0;
unsigned long lastLoopStartUs = 0;
unsigned long lastDebugLoopMs = 0;

void loop()
{
    // Service the particle cloud
    if (pParticleCloud)
        pParticleCloud->Service(false);

    // Monitor how long it takes to go around loop
    if (lastLoopStartUs != 0)
    {
        unsigned long loopTimeUs = micros() - lastLoopStartUs;
        if (loopTimeUs > 0)
        {
            if (loopTimeAvgWinCount == loopTimeAvgWinLen)
            {
                int oldVal = loopTimeAvgWinUs[loopTimeAvgWinHead];
                loopTimeSumUs -= oldVal;
            }
            loopTimeAvgWinUs[loopTimeAvgWinHead++] = loopTimeUs;
            if (loopTimeAvgWinHead >= loopTimeAvgWinLen)
                loopTimeAvgWinHead = 0;
            if (loopTimeAvgWinCount < loopTimeAvgWinLen)
                loopTimeAvgWinCount++;
            loopTimeSumUs += loopTimeUs;
        }
    }
    lastLoopStartUs = micros();
    if (millis() > lastDebugLoopMs + 10000)
    {
        if (loopTimeAvgWinLen > 0)
        {
            Log.info("Avg loop %0.2fuS", 1.0 * loopTimeSumUs / loopTimeAvgWinLen);
        }
        else
        {
            Log.trace("No avg loop time yet");
        }
        lastDebugLoopMs = millis();
    }

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
