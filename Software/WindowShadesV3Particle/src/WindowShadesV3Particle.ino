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
//   Help:           /HELP
//   Set shades:     /SHADECFG/#/name1/name2/name3/name4/name5/name6
//                                        - # = number of shades, name1..6 = shade name
//                                        - responds with same info as Query Status
//
//   Add Notify:     /NO/iii:ppp          - add a request for status notification on a specific ip address and port e.g. 192.168.0.76:25366
//   Wipe config:    /WC/1234             - wipe config EEPROM

//#define DEBUG_CLEAR_EEPROM 1

// Main include for Particle applications
#include "application.h"

// Logging
SerialLogHandler logHandler(LOG_LEVEL_TRACE);

// Utils
#include "Utils.h"

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

// UDP Server
#include "UdpRestApiServer.h"
int udpRestApiServerPort = 7193;
UdpRestApiServer* pUdpRestApiServer = NULL;

// Notifications
#include "NotifyMgr.h"
NotifyMgr* pNotifyMgr = NULL;

// Debug loop used to time main loop
#include "DebugLoopTimer.h"

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

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(AUTOMATIC);
STARTUP(System.enableFeature(FEATURE_RESET_INFO));

// Debug loop timer and callback function
void debugLoopInfoCallback(String& infoStr)
{
  String ipAddr = WiFi.localIP();
  infoStr = String::format(" IP %s FW %s RST %d", ipAddr.c_str(),
            System.version().c_str(), System.resetReason());
}
DebugLoopTimer debugLoopTimer(10000, debugLoopInfoCallback);

// API Support (for web, etc)
#include "RestAPIUtils.h"
#include "RestAPIHelpers.h"
#include "RestAPINetwork.h"
#include "RestAPIShadesManagement.h"
#include "RestAPINotificationManagement.h"

void setupRestAPIEndpoints()
{
    setupRestAPI_Network();
    setupRestAPI_ShadesManagement();
    setupRestAPI_NotificationManager();
    setupRestAPI_Helpers();
}

void setup()
{
    // Construct Window shades first - so outputs are reset
    pWindowShades = new WindowShades(HC595_SER, HC595_SCK, HC595_RCK);

    // Short delay before message
    delay(5000);
    String systemName = "WindowShades";
    Log.info("%s (built %s %s)", systemName.c_str(), __DATE__, __TIME__);

    #ifdef DEBUG_CLEAR_EEPROM
    EEPROM.clear();
    Log.info("EEPROM Cleared");
    #endif

    // EEPROM debug
    size_t length = EEPROM.length();
    Log.info("EEPROM has %d bytes available", length);

    // Initialise the config manager
    configEEPROM.setConfigLocation(EEPROM_CONFIG_LOCATION_STR);

    // Validate settings
    int numShades = configEEPROM.getLong("numShades", -1);
    if (numShades == -1)
    {
        // Clear the config string
        configEEPROM.setConfigData("");
    }
    
    // Particle Cloud
    pParticleCloud = new ParticleCloud(handleReceivedApiStr,
                restHelper_QueryStatus, restHelper_QueryStatusHash,
                5000, systemName);
    pParticleCloud->RegisterVariables();

    // Construct web server
    pWebServer = new RdWebServer();

    // Construct UDP server
    pUdpRestApiServer = new UdpRestApiServer(handleReceivedApiStr);

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

    // Configure UDP server
    if (pUdpRestApiServer)
        pUdpRestApiServer->start(udpRestApiServerPort);

    // Notifications handler
    pNotifyMgr = new NotifyMgr();
    /*pNotifyMgr->addNotifyType(1, restHelper_ReportHealth,
                        restHelper_ReportHealthHash, NotifyMgr::NOTIFY_POST);*/

    // Status LEDs
    pinMode(LED_OP, OUTPUT);
    pinMode(LED_ACT, OUTPUT);

}

void loop()
{
    // Service the particle cloud
    if (pParticleCloud)
        pParticleCloud->Service();

    // Debug loop Timing
    debugLoopTimer.Service();

    // Service the web server
    if (pWebServer)
    {
        pWebServer->service();
        int serverConnState = pWebServer->serverConnState();
        digitalWrite(LED_ACT, serverConnState == RdWebServer::WebServerState::WEB_SERVER_BEGUN);
        int clientConnections = pWebServer->clientConnections();
        digitalWrite(LED_OP, clientConnections > 0);
    }

    // Service UDP server
    if (pUdpRestApiServer)
        pUdpRestApiServer->service();

    // Service the window shades
    if (pWindowShades)
        pWindowShades->service();

    // Service notifications
    if (pNotifyMgr)
        pNotifyMgr->service();

}
