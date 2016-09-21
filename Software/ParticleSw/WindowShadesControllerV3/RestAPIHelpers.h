// Helper functions to implement application specific REST API calls

char* restAPI_ShadesControl(int method, char* cmdStr, char* argStr, char* msgBuffer, int msgLen,
                int contentLen, unsigned char* pPayload, int payloadLen, int splitPayloadPos)
{
    String shadeNumStr = RdWebServer::getNthArgStr(argStr, 0);
    int shadeNum = shadeNumStr.toInt();
    String shadeCmdStr = RdWebServer::getNthArgStr(argStr, 1);
    String shadeDurationStr = RdWebServer::getNthArgStr(argStr, 2);
    if (shadeNum < 1 || shadeNum > pWindowShades->getMaxNumShades())
        return setResultStr(false);
    int shadeIdx = shadeNum-1;
    if (pWindowShades == NULL)
        return setResultStr(false);
    pWindowShades->doCommand(shadeIdx, shadeCmdStr, shadeDurationStr);
    return setResultStr(true);
}

char* restHelper_QueryStatus()
{
    // Get information on status
    String shadeWindowName, numShadesStr, shadeName;
    pConfigDb->getRecValByName(CONFIG_RECIDX_FOR_SHADES, "WINNAME", shadeWindowName);
    pConfigDb->getRecValByName(CONFIG_RECIDX_FOR_SHADES, "NUMSHADES", numShadesStr);
    int numShades = numShadesStr.toInt();
    if (numShades < 1)
        numShades = 1;
    if (numShades > pWindowShades->getMaxNumShades())
        numShades = pWindowShades->getMaxNumShades();
    numShadesStr = String::format("%d", numShades);
    // Return info about IP and WiFi
    String retStr = "{ \"numShades\": \"";
    retStr.concat(numShadesStr);
    retStr.concat("\", \"name\": \"");
    retStr.concat(shadeWindowName);
    // WiFi IP Address
    retStr.concat("\", \"wifiIP\": \"");
    retStr.concat(pWiFiConn->localIPStr());
    // Shades
    retStr.concat("\", \"shades\": [");
    // Add name for each shade
    for (int i = 0; i < numShades; i++)
    {
        pConfigDb->getRecValByName(CONFIG_RECIDX_FOR_SHADES, String::format("SHADENAME%d", i), shadeName);
        retStr.concat("{\"name\": \"");
        retStr.concat(shadeName);
        retStr.concat("\", \"num\": \"");
        retStr.concat(String::format("%d", i+1));
        retStr.concat("\"}");
        if (i != numShades-1)
        {
            retStr.concat(",");
        }
    }
    retStr.concat("]");
    retStr.concat("}");
    retStr.toCharArray(restAPIHelpersBuffer, MAX_REST_API_RETURN_LEN);
    return restAPIHelpersBuffer;
}

char* restAPI_QueryStatus(int method, char*cmdStr, char* argStr, char* msgBuffer, int msgLen,
                int contentLen, unsigned char* pPayload, int payloadLen, int splitPayloadPos)
{
  return restHelper_QueryStatus();
}

char* restAPI_ShadesConfig(int method, char* cmdStr, char* argStr, char* msgBuffer, int msgLen,
                int contentLen, unsigned char* pPayload, int payloadLen, int splitPayloadPos)
{
    String shadeWindowName = RdWebServer::getNthArgStr(argStr, 0);
    pConfigDb->setRecValByName(CONFIG_RECIDX_FOR_SHADES, "WINNAME", shadeWindowName);
    String numShadesStr = RdWebServer::getNthArgStr(argStr, 1);
    int numShades = numShadesStr.toInt();
    if (numShades < 1)
        numShades = 1;
    if (numShades > pWindowShades->getMaxNumShades())
        numShades = pWindowShades->getMaxNumShades();
    numShadesStr = String::format("%d", numShades);
    pConfigDb->setRecValByName(CONFIG_RECIDX_FOR_SHADES, "NUMSHADES", numShadesStr);
    for (int i = 0; i < numShades; i++)
    {
        String shadeName = RdWebServer::getNthArgStr(argStr, 2 + i);
        pConfigDb->setRecValByName(CONFIG_RECIDX_FOR_SHADES, String::format("SHADENAME%d", i), shadeName);
    }
    return restHelper_QueryStatus();
}

char* restAPI_Help(int method, char*cmdStr, char* argStr, char* msgBuffer, int msgLen,
                int contentLen, unsigned char* pPayload, int payloadLen, int splitPayloadPos)
{
  return "/blind/1..N/up|stop|down/pulse|on|off, Q, IW/SSID/PW/WPA2, IP/IPADDR, IP/IPADDR/MASK/GWAY/DNS";
}

