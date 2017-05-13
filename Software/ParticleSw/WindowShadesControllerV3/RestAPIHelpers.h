// Helper functions to implement application specific REST API calls

char *restAPI_Help(int method, char *cmdStr, char *argStr, char *msgBuffer, int msgLen,
                   int contentLen, unsigned char *pPayload, int payloadLen, int splitPayloadPos)
{
    return "/blind/1..N/up|stop|down/pulse|on|off, Q, IW/SSID/PW/WPA2, IP/IPADDR, IP/IPADDR/MASK/GWAY/DNS";
}

char *restHelper_QueryStatus()
{
    // Get information on status
    String shadeWindowName = configEEPROM.getString("WINNAME", "");
    int numShades = configEEPROM.getLong("NUMSHADES", 0);
    if (numShades < 0)
    {
        numShades = 0;
    }
    if (numShades > pWindowShades->getMaxNumShades())
    {
        numShades = pWindowShades->getMaxNumShades();
    }
    String numShadesStr = String::format("%d", numShades);

    // // Return info about IP and WiFi
    String retStr = "{ \"numShades\": \"";
    retStr.concat(numShadesStr);
    retStr.concat("\", \"name\": \"");
    retStr.concat(shadeWindowName);
    // WiFi IP Address
    retStr.concat("\", \"wifiIP\": \"");
    retStr.concat(pParticleCloud->localIPStr());
    // Light sensors
    const int lightSensorPins[] = { SENSE_A0, SENSE_A1, SENSE_A2 };
    const int numPins           = sizeof(lightSensorPins) / sizeof(int);
    retStr.concat("\", \"sens\": [");
    for (int i = 0; i < numPins; i++)
    {
        retStr.concat("{\"i\": \"");
        retStr.concat(String::format("%d", i));
        retStr.concat("\",\"v\":\"");
        retStr.concat(String::format("%d", analogRead(lightSensorPins[i])));
        retStr.concat("\"}");
        if (i != numPins - 1)
        {
            retStr.concat(",");
        }
    }
    retStr.concat("]");
    // Shades
    retStr.concat(", \"shades\": [");
    // Add name for each shade
    for (int i = 0; i < numShades; i++)
    {
        String shadeName = configEEPROM.getString(String::format("SHADENAME%d", i), "");
        retStr.concat("{\"name\": \"");
        retStr.concat(shadeName);
        retStr.concat("\", \"num\": \"");
        retStr.concat(String::format("%d", i + 1));
        retStr.concat("\"}");
        if (i != numShades - 1)
        {
            retStr.concat(",");
        }
    }
    retStr.concat("]");
    retStr.concat("}");
    retStr.toCharArray(restAPIHelpersBuffer, MAX_REST_API_RETURN_LEN);
    return restAPIHelpersBuffer;
}


char *restAPI_QueryStatus(int method, char *cmdStr, char *argStr, char *msgBuffer, int msgLen,
                          int contentLen, unsigned char *pPayload, int payloadLen, int splitPayloadPos)
{
    return restHelper_QueryStatus();
}


char *restAPI_ShadesControl(int method, char *cmdStr, char *argStr, char *msgBuffer, int msgLen,
                            int contentLen, unsigned char *pPayload, int payloadLen, int splitPayloadPos)
{
    String shadeNumStr      = RestAPIEndpoints::getNthArgStr(argStr, 0);
    int    shadeNum         = shadeNumStr.toInt();
    String shadeCmdStr      = RestAPIEndpoints::getNthArgStr(argStr, 1);
    String shadeDurationStr = RestAPIEndpoints::getNthArgStr(argStr, 2);

    if ((shadeNum < 1) || (shadeNum > pWindowShades->getMaxNumShades()))
    {
        return restAPIsetResultStr(false);
    }
    int shadeIdx = shadeNum - 1;
    if (pWindowShades == NULL)
    {
        return restAPIsetResultStr(false);
    }
    pWindowShades->doCommand(shadeIdx, shadeCmdStr, shadeDurationStr);
    return restAPIsetResultStr(true);
}

char *restAPI_ShadesConfig(int method, char *cmdStr, char *argStr, char *msgBuffer, int msgLen,
                           int contentLen, unsigned char *pPayload, int payloadLen, int splitPayloadPos)
{
    String shadeWindowName = RestAPIEndpoints::getNthArgStr(argStr, 0);

//    pConfigDb->setRecValByName(CONFIG_RECIDX_FOR_SHADES, "WINNAME", shadeWindowName);
    String numShadesStr = RestAPIEndpoints::getNthArgStr(argStr, 1);
    int    numShades    = numShadesStr.toInt();
    if (numShades < 1)
    {
        numShades = 1;
    }
    if (numShades > pWindowShades->getMaxNumShades())
    {
        numShades = pWindowShades->getMaxNumShades();
    }
    numShadesStr = String::format("%d", numShades);
//    pConfigDb->setRecValByName(CONFIG_RECIDX_FOR_SHADES, "NUMSHADES", numShadesStr);
    for (int i = 0; i < numShades; i++)
    {
        String shadeName = RestAPIEndpoints::getNthArgStr(argStr, 2 + i);
//        pConfigDb->setRecValByName(CONFIG_RECIDX_FOR_SHADES, String::format("SHADENAME%d", i), shadeName);
    }
    return restHelper_QueryStatus();
}
