// Helper functions to implement WiFi and IP REST API calls

char* restAPI_Wifi(int method, char*cmdStr, char* argStr, char* msgBuffer, int msgLen,
                int contentLen, unsigned char* pPayload, int payloadLen, int splitPayloadPos)
{
    bool rslt = true;
    // Check for clear stored SSIDs
    if (RdWebServer::getNumArgs(argStr) == 0)
    {
        WiFi.clearCredentials();
        return setResultStr(rslt);
    }
    // Get params
    wifiSSID = RdWebServer::getNthArgStr(argStr, 0);
    if (wifiSSID.length() == 0)
        pConfigDb->getRecValByName(CONFIG_RECIDX_FOR_INTERNET, "SSID", wifiSSID);
    else
        rslt &= pConfigDb->setRecValByName(CONFIG_RECIDX_FOR_INTERNET, "SSID", wifiSSID);
    wifiPassword = RdWebServer::getNthArgStr(argStr, 1);
    if (wifiPassword.length() == 0)
        pConfigDb->getRecValByName(CONFIG_RECIDX_FOR_INTERNET, "PW", wifiPassword);
    else
        rslt &= pConfigDb->setRecValByName(CONFIG_RECIDX_FOR_INTERNET, "PW", wifiPassword);
    wifiConnMethod = RdWebServer::getNthArgStr(argStr, 2);
    if (wifiConnMethod.length() == 0)
        pConfigDb->getRecValByName(CONFIG_RECIDX_FOR_INTERNET, "METH", wifiConnMethod);
    else
        rslt &= pConfigDb->setRecValByName(CONFIG_RECIDX_FOR_INTERNET, "METH", wifiConnMethod);
    // Use the data to connect
    pWiFiConn->connect(wifiSSID, wifiPassword, wifiConnMethod, wifiIPAddr, wifiSubnetMask, wifiGatewayIP, wifiDNSIP);
    return setResultStr(rslt);
}

char* restAPI_NetworkIP(int method, char*cmdStr, char* argStr, char* msgBuffer, int msgLen,
                int contentLen, unsigned char* pPayload, int payloadLen, int splitPayloadPos)
{
  // Get param
  String ipAddr = RdWebServer::getNthArgStr(argStr, 0);
  // Check for query
  if (ipAddr.trim().length() == 0)
  {
    String configSSID, configConnMethod, configIPAddr, configSubnetMask, configGatewayIP, configDNSIP;
    pConfigDb->getRecValByName(CONFIG_RECIDX_FOR_INTERNET, "SSID", configSSID);
    pConfigDb->getRecValByName(CONFIG_RECIDX_FOR_INTERNET, "METH", configConnMethod);
    pConfigDb->getRecValByName(CONFIG_RECIDX_FOR_INTERNET, "IP", configIPAddr);
    pConfigDb->getRecValByName(CONFIG_RECIDX_FOR_INTERNET, "MASK", configSubnetMask);
    pConfigDb->getRecValByName(CONFIG_RECIDX_FOR_INTERNET, "GATE", configGatewayIP);
    pConfigDb->getRecValByName(CONFIG_RECIDX_FOR_INTERNET, "DNS", configDNSIP);
    // Return info about IP and WiFi
    String retStr = "{ \"wifiConn\": \"";
    retStr.concat(pWiFiConn->connStateStr());
    retStr.concat("\", \"wifiSSID\": \"");
    retStr.concat(pWiFiConn->SSID());
    retStr.concat("\", \"configSSID\": \"");
    retStr.concat(configSSID);
    retStr.concat("\", \"configConnMethod\": \"");
    retStr.concat(configConnMethod);
    retStr.concat("\", \"wifiBSSID\": \"");
    retStr.concat(pWiFiConn->BSSIDStr());
    retStr.concat("\", \"wifiMAC\": \"");
    retStr.concat(pWiFiConn->MACAddrStr());
    retStr.concat("\", \"wifiRSSI\": \"");
    retStr.concat(pWiFiConn->RSSI());
    retStr.concat("\", \"wifiIP\": \"");
    retStr.concat(pWiFiConn->localIPStr());
    retStr.concat("\", \"configIP\": \"");
    retStr.concat(configIPAddr);
    retStr.concat("\", \"configMask\": \"");
    retStr.concat(configSubnetMask);
    retStr.concat("\", \"configGateway\": \"");
    retStr.concat(configGatewayIP);
    retStr.concat("\", \"configDNS\": \"");
    retStr.concat(configDNSIP);
    retStr.concat("\", \"serverConn\": \"");
    retStr.concat(pWebServer->connStateStr());
    retStr.concat("\" }");
    retStr.toCharArray(restAPIHelpersBuffer, MAX_REST_API_RETURN_LEN);
    return restAPIHelpersBuffer;
  }
  // Store IP
  bool rslt = pConfigDb->setRecValByName(CONFIG_RECIDX_FOR_INTERNET, "IP", ipAddr);
  wifiIPAddr = ipAddr;
  // Get other params
  wifiSubnetMask = RdWebServer::getNthArgStr(argStr, 1);
  if (wifiSubnetMask.length() == 0)
    wifiSubnetMask = "255.255.255.0";
  wifiGatewayIP = RdWebServer::getNthArgStr(argStr, 2);
  if (wifiGatewayIP.length() == 0)
  {
    wifiGatewayIP = wifiIPAddr.substring(0,wifiIPAddr.lastIndexOf("."));
    wifiGatewayIP.concat(".1");
  }
  wifiDNSIP = RdWebServer::getNthArgStr(argStr, 3);
  if (wifiDNSIP.length() == 0)
    wifiDNSIP = wifiGatewayIP;
  // Store in config db
  rslt = rslt & pConfigDb->setRecValByName(CONFIG_RECIDX_FOR_INTERNET, "MASK", wifiSubnetMask);
  rslt = rslt & pConfigDb->setRecValByName(CONFIG_RECIDX_FOR_INTERNET, "GATE", wifiGatewayIP);
  rslt = rslt & pConfigDb->setRecValByName(CONFIG_RECIDX_FOR_INTERNET, "DNS", wifiDNSIP);
  // reconnect WiFi
  pWiFiConn->connect(wifiSSID, wifiPassword, wifiConnMethod, wifiIPAddr, wifiSubnetMask, wifiGatewayIP, wifiDNSIP);
  return setResultStr(rslt);
}
