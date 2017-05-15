// Helper functions to implement WiFi and IP REST API calls
// Rob Dobson 2012-2016

// Set WiFi information via API
char *restAPI_Wifi(int method, char *cmdStr, char *argStr, char *msgBuffer, int msgLen,
                   int contentLen, unsigned char *pPayload, int payloadLen, int splitPayloadPos)
{
    bool rslt = true;

    // Check for clear stored SSIDs
    if (RestAPIEndpoints::getNumArgs(argStr) == 0)
    {
        Log.info("Cleared WiFi Credentials");
        WiFi.clearCredentials();
        return restAPIsetResultStr(rslt);
    }
    // Get wifi params
    String wifiSSID = RestAPIEndpoints::getNthArgStr(argStr, 0);
    String wifiPassword   = RestAPIEndpoints::getNthArgStr(argStr, 1);
    String wifiAuth = RestAPIEndpoints::getNthArgStr(argStr, 2);
    if (wifiSSID.length() > 0)
    {
        if (wifiAuth == "OPEN")
        {
            Log.info("SetWiFiCredentials: OPEN, %s", wifiSSID.c_str());
            WiFi.setCredentials(wifiSSID);
        }
        else if (wifiAuth == "WEP")
        {
            Log.info("SetWiFiCredentials: WEP, %s, %s", wifiSSID.c_str(), wifiPassword.c_str());
            WiFi.setCredentials(wifiSSID, wifiPassword, WEP);
        }
        else if (wifiAuth == "WPA-TKIP")
        {
            Log.info("SetWiFiCredentials: WPA-TKIP, %s, %s", wifiSSID.c_str(), wifiPassword.c_str());
            WiFi.setCredentials(wifiSSID, wifiPassword, WPA, WLAN_CIPHER_TKIP);
        }
        else if (wifiAuth == "WPA2-TKIP")
        {
            Log.info("SetWiFiCredentials: WPA2-TKIP, %s, %s", wifiSSID.c_str(), wifiPassword.c_str());
            WiFi.setCredentials(wifiSSID, wifiPassword, WPA2, WLAN_CIPHER_TKIP);
        }
        else if (wifiAuth == "WPA2-AES")
        {
            Log.info("SetWiFiCredentials: WPA2-AES, %s, %s", wifiSSID.c_str(), wifiPassword.c_str());
            WiFi.setCredentials(wifiSSID, wifiPassword, WPA2, WLAN_CIPHER_AES);
        }
        else if ((wifiAuth == "WPA2") || (wifiAuth == "WPA2-AES-TKIP"))
        {
            Log.info("SetWiFiCredentials: WPA2-AES-TKIP, %s, %s", wifiSSID.c_str(), wifiPassword.c_str());
            WiFi.setCredentials(wifiSSID, wifiPassword, WPA2, WLAN_CIPHER_AES_TKIP);
        }
        else
        {
            Log.info("SetWiFiCredentials: Doing nothing - unknown Auth", wifiAuth.c_str());
            rslt = false;
        }
    }
    else
    {
        Log.info("SetWiFiCredentials: Doing nothing - no SSID %s, pw %s, auth %s",
                        wifiSSID.c_str(), wifiPassword.c_str(), wifiAuth.c_str());
        rslt = false;
    }
    // Result
    return restAPIsetResultStr(rslt);
}


// Get or set IP details via API
char *restAPI_NetworkIP(int method, char *cmdStr, char *argStr, char *msgBuffer, int msgLen,
                        int contentLen, unsigned char *pPayload, int payloadLen, int splitPayloadPos)
{
    // Get param
    String ipAddr = RestAPIEndpoints::getNthArgStr(argStr, 0);
    ipAddr = ipAddr.trim();

    // Check for query
    if (ipAddr.length() == 0)
    {
        // Return info about IP and WiFi
        String retStr = "{ \"wifiConn\": \"";
        retStr.concat(pParticleCloud->connStateStr());
        retStr.concat("\", \"wifiSSID\": \"");
        retStr.concat(WiFi.SSID());
        retStr.concat("\", \"wifiBSSID\": \"");
        retStr.concat(pParticleCloud->BSSIDStr());
        retStr.concat("\", \"wifiMAC\": \"");
        retStr.concat(pParticleCloud->MACAddrStr());
        retStr.concat("\", \"wifiRSSI\": \"");
        retStr.concat(WiFi.RSSI());
        retStr.concat("\", \"wifiIP\": \"");
        retStr.concat(pParticleCloud->localIPStr());
        retStr.concat("\", \"serverConn\": \"");
        retStr.concat(pWebServer->connStateStr());
        retStr.concat("\" }");
        retStr.toCharArray(restAPIHelpersBuffer, MAX_REST_API_RETURN_LEN);
        return restAPIHelpersBuffer;
    }

    // Alternatively set the IP details

    // If no IP address specified then use dynamic IP
    if (!isdigit(ipAddr.charAt(0)))
    {
        WiFi.useDynamicIP();
        return restAPIsetResultStr(true);
    }

    // See if we can set a static IP address
    String wifiSubnetMask = RestAPIEndpoints::getNthArgStr(argStr, 1);
    if (wifiSubnetMask.length() == 0)
    {
        wifiSubnetMask = "255.255.255.0";
    }
    String wifiGatewayIP = RestAPIEndpoints::getNthArgStr(argStr, 2);
    if (wifiGatewayIP.length() == 0)
    {
        if (ipAddr.equalsIgnoreCase("auto"))
        {
            wifiGatewayIP = "";
        }
        else
        {
            wifiGatewayIP = ipAddr.substring(0, ipAddr.lastIndexOf("."));
            wifiGatewayIP.concat(".1");
        }
    }
    String wifiDNSIP = RestAPIEndpoints::getNthArgStr(argStr, 3);
    if (wifiDNSIP.length() == 0)
    {
        wifiDNSIP = wifiGatewayIP;
    }

    // Select dynamic or static IP based on whether info is valid
    unsigned long wifiIpAddr = Utils::convIPStrToAddr(ipAddr);
    String debugStr;
    if (wifiIpAddr == Utils::INADDR_NONE)
    {
        Log.info("NetworkIP: Dynamic IP");
        WiFi.useDynamicIP();
    }
    else
    {
        unsigned long ipMask = Utils::convIPStrToAddr(wifiSubnetMask);
        unsigned long ipGate = Utils::convIPStrToAddr(wifiGatewayIP);
        unsigned long ipDNS = Utils::convIPStrToAddr(wifiDNSIP);
        WiFi.setStaticIP(IPAddress(wifiIpAddr), IPAddress(ipMask), IPAddress(ipGate), IPAddress(ipDNS));
        WiFi.useStaticIP();
        Log.info("NetworkIP: Static IP %s, %s, %s, %s", ipAddr.c_str(),
                        wifiSubnetMask.c_str(), wifiGatewayIP.c_str(), wifiDNSIP.c_str());
    }
    return restAPIsetResultStr(true);
}
