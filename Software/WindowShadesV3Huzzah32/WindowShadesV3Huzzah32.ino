// Window Shades V3.3
// Rob Dobson 2012-2018

// API used for web, MQTT and BLE - very short to allow over BLE UART
//   Query status:    /Q                      - returns network status, number of blinds etc
//   Set WiFi:        /W/ss/pp/hh             - ss = ssid, pp = password, hh = hostname - assumes WPA2 - does not clear previous WiFi so clear first if required
//   Clear WiFi:      /WC                     - clears all stored SSID, etc
//   Ext antenna:     /WAX                    - external antenna for WiFi
//   Int antenna:     /WAI                    - internal antenna for WiFi
//   Wipe config:     /WIPEALL                - wipe shades config
//   Help:            /HELP
//   Shade Move:      /SHADE/#/cmd/duration   - # is the shade number,
//                                            cmd == "up", "down", "stop", "setuplimit", "setdownlimit", "resetmemory"
//                                            duration is "pulse", "on", "off"
//   Shade Move:      /BLIND/#/cmd/duration   - alias of above
//   Set shades:      /SHADECFG/#/name1/name2/name3/name4/name5/name6
//                                            - # = number of shades, name1..6 = shade name
//                                            - responds with same info as Query Status
//

// Logging
#include <ArduinoLog.h>

// WiFi
#include <WiFi.h>

// Utils
#include "Utils.h"

// Config
#include "ConfigNVS.h"

// WiFi Manager
#include "WiFiManager.h"
WiFiManager wifiManager;

// API Endpoints
#include "RestAPIEndpoints.h"
RestAPIEndpoints restAPIEndpoints;

// Serial console - for configuration
#include "SerialConsole.h"

// Web server
#include "RdWebServer.h"
const int webServerPort = 80;
RdWebServer* pWebServer = NULL;
#include "GenResources.h"

// Config for window shades
ConfigNVS shadesConfig("shades", 10000);

// Config for WiFi
ConfigNVS wifiConfig("wifi", 100);

// ParticleCloud support
//#include "ParticleCloud.h"
//ParticleCloud* pParticleCloud = NULL;
//
//// UDP Server
//#include "UdpRestApiServer.h"
//int udpRestApiServerPort = 7193;
//UdpRestApiServer* pUdpRestApiServer = NULL;
//
//// Notifications
//#include "NotifyMgr.h"
//NotifyMgr* pNotifyMgr = NULL;

// Window shades
#include "WindowShades.h"
WindowShades* pWindowShades = NULL;
const int HC595_SER = 21;               // 75HC595 pins
const int HC595_SCK = A5;               //
const int HC595_LATCH = 16;             //
const int HC595_RST = 17;               //
const int LED_OP = 13;
const int LED_ACT = 15;
const int SENSE_A0 = A0;
const int SENSE_A1 = A1;
const int SENSE_A2 = A2;

// Debug loop used to time main loop
#include "DebugLoopTimer.h"

// Debug loop timer and callback function
void debugLoopInfoCallback(String& infoStr)
{
    infoStr = "Shades SSID " + WiFi.SSID() + " IP " + WiFi.localIP().toString();
}
DebugLoopTimer debugLoopTimer(10000, debugLoopInfoCallback);

//// API Support (for web, etc)
#include "RestAPIUtils.h"
#include "RestAPINetwork.h"
#include "RestAPISystem.h"
//#include "RestAPINotificationManagement.h"
#include "RestAPIHelpers.h"
#include "RestAPIShadesManagement.h"

// Serial console
SerialConsole serialConsole(0, handleReceivedApiStr);

void setupRestAPIEndpoints()
{
    setupRestAPI_Network();
    setupRestAPI_System();
    setupRestAPI_ShadesManagement();
//    setupRestAPI_NotificationManager();
    setupRestAPI_Helpers();
}

void setup()
{
    // Logging
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial);

    // Construct Window shades first - so outputs are reset
    pWindowShades = new WindowShades(HC595_SER, HC595_SCK, HC595_LATCH, HC595_RST);

    // Message
    String systemName = "WindowShades";
    Log.notice(F("%s (built %s %s)"CR), systemName.c_str(), __DATE__, __TIME__);

    // Initialise the config managers
    shadesConfig.setup();
    wifiConfig.setup();

    // Override the system name if provided
    systemName = shadesConfig.getString("name", systemName.c_str());

    // WiFi Manager
    wifiManager.setup(&wifiConfig, systemName.c_str());
    
//
//    // Particle Cloud
//    pParticleCloud = new ParticleCloud(handleReceivedApiStr,
//                restHelper_ReportHealth, restHelper_ReportHealthHash,
//                5000, systemName);
//    pParticleCloud->registerVariables();

    // Construct web server
    pWebServer = new RdWebServer();

//    // Construct UDP server
//    pUdpRestApiServer = new UdpRestApiServer(handleReceivedApiStr);

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

//    // Configure UDP server
//    if (pUdpRestApiServer)
//        pUdpRestApiServer->start(udpRestApiServerPort);
//
//    // Notifications handler
//    pNotifyMgr = new NotifyMgr();
//    pNotifyMgr->addNotifyType(1, restHelper_ReportHealth,
//                        restHelper_ReportHealthHash, NotifyMgr::NOTIFY_POST);

    // Status LEDs
    pinMode(LED_OP, OUTPUT);
    pinMode(LED_ACT, OUTPUT);

    // Add debug blocks
    debugLoopTimer.blockAdd(0, "Web");
    debugLoopTimer.blockAdd(1, "Serial");

}

void loop()
{
//    // Service the particle cloud
//    if (pParticleCloud)
//        pParticleCloud->service();

    // Debug loop Timing
    debugLoopTimer.service();

    // Service WiFi
    wifiManager.service();

    // Service the web server
    if (wifiManager.isConnected())
    {
        if (pWebServer)
        {
            debugLoopTimer.blockStart(0);
            pWebServer->service();
            debugLoopTimer.blockEnd(0);
    //        int serverConnState = pWebServer->serverConnState();
    //        digitalWrite(LED_ACT, serverConnState == RdWebServer::WebServerState::WEB_SERVER_BEGUN);
    //        int clientConnections = pWebServer->clientConnections();
    //        digitalWrite(LED_OP, clientConnections > 0);
        }
    }

     // Serial console
    debugLoopTimer.blockStart(1);
    serialConsole.service();
    debugLoopTimer.blockEnd(1);

//    // Service UDP server
//    if (pUdpRestApiServer)
//        pUdpRestApiServer->service();

    // Service the window shades
    if (pWindowShades)
        pWindowShades->service();

//    // Service notifications
//    if (pNotifyMgr)
//        pNotifyMgr->service();

}
