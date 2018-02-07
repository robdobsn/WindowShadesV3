// Helper functions to implement REST API calls
// Rob Dobson 2012-2016

unsigned long restHelper_QueryStatusHash()
{
    return ParticleCloud::HASH_VALUE_FOR_ALWAYS_REPORT;
}

void restHelper_QueryStatus(const char* pIdStr,
                String* pInitialContentJsonElementList, String& retStr)
{
    // Get information on status
    String shadeWindowName = configEEPROM.getString("name", "");
    int numShades = configEEPROM.getLong("numShades", 0);
    if (numShades < 1)
    {
        numShades = 1;
    }
    if (numShades > pWindowShades->getMaxNumShades())
    {
        numShades = pWindowShades->getMaxNumShades();
    }
    String numShadesStr = String::format("%d", numShades);

    // // Return info about IP and WiFi
    retStr = "\"numShades\": \"";
    retStr.concat(numShadesStr);
    retStr.concat("\", \"name\": \"");
    retStr.concat(shadeWindowName);
    retStr.concat("\",");

    // System status
    uint16_t resetReason = System.resetReason();
    uint32_t systemVersion = System.versionNumber();
    String sOut = String::format("\"rst\":\"%d\",\"ver\":\"%08x\",\"blddt\":\"%s\",\"bldtm\":\"%s\",",
                    resetReason,
                    systemVersion,
                    __DATE__, __TIME__);
    retStr.concat(sOut);

    // WiFi IP Address
    retStr.concat("\"wifiIP\": \"");
    String localIPStr = WiFi.localIP();
    retStr.concat(localIPStr.c_str());
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
        String shadeName = configEEPROM.getString(String::format("sh%d", i), "");
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
    retStr = "{" + retStr +
        (pInitialContentJsonElementList != NULL ? "," : "") +
        (pInitialContentJsonElementList != NULL ? *pInitialContentJsonElementList : "") + "}";
    retStr = retStr.replace('\'', '\"');
}

void restAPI_QueryStatus(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    restHelper_QueryStatus(NULL, NULL, retStr);
}

void restAPI_WipeConfig(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    EEPROM.clear();
    configEEPROM.readFromEEPROM();
    Serial.println("EEPROM Cleared");
    restAPI_setResultStr(retStr, true);
}

void restAPI_Reset(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    System.reset();
    restAPI_setResultStr(retStr, true);
}

void restAPI_Help(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    retStr = "/blind/1..N/up|stop|down/pulse|on|off, Q";
}

// Register REST API commands
void setupRestAPI_Helpers()
{
    // Query
    restAPIEndpoints.addEndpoint("Q", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_QueryStatus, "");

    // Reset device
    restAPIEndpoints.addEndpoint("RESET", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_Reset, "");

    // Wipe config
    restAPIEndpoints.addEndpoint("WIPEALL", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_WipeConfig, "");

    // Wipe config
    restAPIEndpoints.addEndpoint("HELP", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_Help, "");
}
