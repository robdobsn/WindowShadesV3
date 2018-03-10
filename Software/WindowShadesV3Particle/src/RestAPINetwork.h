// Helper functions to implement WiFi and IP REST API calls
// Rob Dobson 2012-2016

int restHelper_ReportHealth_Network(int bitPosStart, unsigned long* pOutHash, String* pOutStr)
{
    // Generate hash if required
    if (pOutHash)
    {
        unsigned long hashVal = WiFi.ready();
        #ifdef ENABLE_WEB_SERVER
        hashVal += pWebServer->serverConnState() << 4;
        #endif
        hashVal = hashVal << bitPosStart;
        *pOutHash += hashVal;
        *pOutHash ^= WiFi.localIP();
    }
    // Generate JSON string if needed
    if (pOutStr)
    {
        String localIPStr = WiFi.localIP();
        byte mac[6];
        WiFi.macAddress(mac);
        String macStr = String::format("%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        String sOut = String::format("'wifiIP':'%s','wifiConn':'%c','webConn':'%c','ssid':'%s','MAC':'%s','RSSI':'%d','VER':'%s'",
                    localIPStr.c_str(),
                    WiFi.ready() ? 'C' : '0',
                    #ifdef ENABLE_WEB_SERVER
                        pWebServer->connStateChar(),
                    #else
                        '*',
                    #endif
                    WiFi.SSID(),
                    macStr.c_str(),
                    WiFi.RSSI(),
                    System.version().c_str());
        *pOutStr = sOut;
    }
    // Return number of bits in hash
    return 8;
}

String overrideWiFiSSID, overrideWiFiPassword;
void restAPI_WifiSet(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    bool rslt = false;
    // Get SSID and set override if not empty
    String ssid = RestAPIEndpoints::getNthArgStr(apiMsg._pArgStr, 0);
    if (ssid.length() != 0)
        overrideWiFiSSID = ssid;
    // Get pw and set override if not empty
    String pw = RestAPIEndpoints::getNthArgStr(apiMsg._pArgStr, 1);
    if (pw.length() != 0)
        overrideWiFiPassword = pw;
    // Check if both SSID and pw have now been set
    if (overrideWiFiSSID.length() != 0 && overrideWiFiPassword.length() != 0)
    {
        WiFi.setCredentials(overrideWiFiSSID, overrideWiFiPassword);
        Log.info("WiFi Credentials Added SSID %s", overrideWiFiSSID.c_str());
        overrideWiFiSSID = "";
        overrideWiFiPassword = "";
        rslt = true;
    }
    restAPI_setResultStr(retStr, rslt);
}

void restAPI_WifiClear(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    // Clear stored SSIDs
    WiFi.clearCredentials();
    Log.info("Cleared WiFi Credentials");
    restAPI_setResultStr(retStr, true);
}

void restAPI_WifiExtAntenna(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    WiFi.selectAntenna(ANT_EXTERNAL);
    Log.info("Set to external antenna");
    restAPI_setResultStr(retStr, true);
}

void restAPI_WifiIntAntenna(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    WiFi.selectAntenna(ANT_INTERNAL);
    Log.info("Set to external antenna");
    restAPI_setResultStr(retStr, true);
}

void restAPI_WebServRestart(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    if (pWebServer)
    {
        pWebServer->start(webServerPort);
    }
    restAPI_setResultStr(retStr, true);
}

// Register REST API commands
void setupRestAPI_Network()
{
    restAPIEndpoints.addEndpoint("WC", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_WifiClear, "", "");
    restAPIEndpoints.addEndpoint("W", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_WifiSet, "", "");
    restAPIEndpoints.addEndpoint("WAX", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_WifiExtAntenna, "", "");
    restAPIEndpoints.addEndpoint("WAI", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_WifiIntAntenna, "", "");
    restAPIEndpoints.addEndpoint("WSRST", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_WebServRestart, "", "");
}
