// Helper functions to implement WiFi and IP REST API calls
// Rob Dobson 2012-2018

String getWifiStatusStr()
{
  if (WiFi.status() == WL_CONNECTED) return "C";
  if (WiFi.status() == WL_NO_SHIELD) return "4";
  if (WiFi.status() == WL_IDLE_STATUS) return "I";
  if (WiFi.status() == WL_NO_SSID_AVAIL) return "N";
  if (WiFi.status() == WL_SCAN_COMPLETED) return "S";
  if (WiFi.status() == WL_CONNECT_FAILED) return "F";
  if (WiFi.status() == WL_CONNECTION_LOST) return "L";
  return "D";
}

int restHelper_ReportHealth_Network(int bitPosStart, unsigned long* pOutHash, String* pOutStr)
{
    // Generate hash if required
    if (pOutHash)
    {
        unsigned long hashVal = (WiFi.status() == WL_CONNECTED);
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
        byte mac[6];
        WiFi.macAddress(mac);
        String macStr = String(mac[0], HEX) + ":" + String(mac[1],HEX) + ":" + String(mac[2], HEX) + ":" +
                        String(mac[3], HEX) + ":" + String(mac[4],HEX) + ":" + String(mac[5], HEX);
        String sOut = "\"wifiIP\":\"" + WiFi.localIP().toString() + "\",\"wifiConn\":\"" + getWifiStatusStr() + "\",\"webConn\":\"" + 
                        String(pWebServer->connStateChar()) + "\",\"ssid\":\"" + WiFi.SSID() + "\",\"MAC\":\"" + macStr + "\",\"RSSI\":" + 
                        String(WiFi.RSSI());
        *pOutStr = sOut;
    }
    // Return number of bits in hash
    return 8;
}

void restAPI_WifiSet(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    bool rslt = false;
    // Get SSID 
    String ssid = RestAPIEndpoints::getNthArgStr(apiMsg._pArgStr, 0);
    // Get pw 
    String pw = RestAPIEndpoints::getNthArgStr(apiMsg._pArgStr, 1);
    // Get hostname
    String hostname = RestAPIEndpoints::getNthArgStr(apiMsg._pArgStr, 2);
    // Check if both SSID and pw have now been set
    if (ssid.length() != 0 && pw.length() != 0)
    {
        wifiManager.setCredentials(ssid, pw, hostname);
        Log.notice(F("WiFi Credentials Added SSID %s"CR), ssid.c_str());
        rslt = true;
    }
    restAPI_setResultStr(retStr, rslt);
}

void restAPI_WifiClear(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    // Clear stored SSIDs
    wifiManager.clearCredentials();
    Log.notice(F("Cleared WiFi Credentials"CR));
    restAPI_setResultStr(retStr, true);
}

void restAPI_WifiExtAntenna(RestAPIEndpointMsg& apiMsg, String& retStr)
{
//    WiFi.selectAntenna(ANT_EXTERNAL);
    Log.notice(F("Not Supported: Set external antenna"CR));
    restAPI_setResultStr(retStr, false);
}

void restAPI_WifiIntAntenna(RestAPIEndpointMsg& apiMsg, String& retStr)
{
//    WiFi.selectAntenna(ANT_INTERNAL);
    Log.notice(F("Not Supported: Set internal antenna"CR));
    restAPI_setResultStr(retStr, false);
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
