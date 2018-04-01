// Helper functions to implement REST API calls
// Rob Dobson 2012-2018

int restHelper_ReportHealth_Shades(int bitPosStart, bool incRelockInStr,
                                   unsigned long* pOutHash, String* pOutStr_jsonMin,
                                   String* pOutStr_urlMin)
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

    // Generate hash if required - no changes
    if (pOutHash)
    {
        *pOutHash += 0;
    }

    // Generate JSON string if needed
    if (pOutStr_jsonMin)
    {
        // Light sensors
        String lightSensorStr;
        const int lightSensorPins[] = { SENSE_A0, SENSE_A1, SENSE_A2 };
        const int numPins           = sizeof(lightSensorPins) / sizeof(int);
        lightSensorStr = "\"sens\": [";
        for (int i = 0; i < numPins; i++)
        {
            lightSensorStr.concat("{\"i\": \"");
            lightSensorStr.concat(String::format("%d", i));
            lightSensorStr.concat("\",\"v\":\"");
            lightSensorStr.concat(String::format("%d", analogRead(lightSensorPins[i])));
            lightSensorStr.concat("\"}");
            if (i != numPins - 1)
            {
                lightSensorStr.concat(",");
            }
        }
        lightSensorStr.concat("]");
        // Shades
        String shadesDetailStr;
        shadesDetailStr = "\"shades\": [";
        // Add name for each shade
        for (int i = 0; i < numShades; i++)
        {
            String shadeName = configEEPROM.getString(String::format("sh%d", i), "");
            shadesDetailStr.concat("{\"name\": \"");
            shadesDetailStr.concat(shadeName);
            shadesDetailStr.concat("\", \"num\": \"");
            shadesDetailStr.concat(String::format("%d", i + 1));
            shadesDetailStr.concat("\"}");
            if (i != numShades - 1)
            {
                shadesDetailStr.concat(",");
            }
        }
        shadesDetailStr.concat("]");
        // Shade name and number
        String shadesStr = String::format("\"numShades\":\"%s\", \"name\": \"%s\"",
                numShadesStr.c_str(), shadeWindowName.c_str());
        // Compile output string
        String sOut = String::format("%s,%s,%s",
                                     shadesStr.c_str(), shadesDetailStr.c_str(), lightSensorStr.c_str());
        *pOutStr_jsonMin = sOut;
    }
    // Return number of bits in hash
    return 1;
}

unsigned long restHelper_ReportHealthHash()
{
    unsigned long hashVal = 0;
    int hashUsedBits = 0;
    hashUsedBits += restHelper_ReportHealth_Shades(0, false, &hashVal, NULL, NULL);
    hashUsedBits += restHelper_ReportHealth_System(hashUsedBits, &hashVal, NULL, NULL);
    hashUsedBits += restHelper_ReportHealth_Network(hashUsedBits, &hashVal, NULL);
    // Log.info("RepHealthHash %ld", hashVal);
    return hashVal;
}

void restHelper_ReportHealth(const char* pIdStr,
                             String* pInitialContentJsonElementList, String& retStr)
{
    String innerJsonStr;
    int hashUsedBits = 0;
    // System health
    String healthStrSystem;
    hashUsedBits += restHelper_ReportHealth_System(hashUsedBits, NULL, &healthStrSystem, NULL);
    if (innerJsonStr.length() > 0)
        innerJsonStr += ",";
    innerJsonStr += healthStrSystem;
    // Network information
    String healthStrNetwork;
    hashUsedBits += restHelper_ReportHealth_Network(hashUsedBits, NULL, &healthStrNetwork);
    if (innerJsonStr.length() > 0)
        innerJsonStr += ",";
    innerJsonStr += healthStrNetwork;
    // Shades info
    String healthStr;
    hashUsedBits += restHelper_ReportHealth_Shades(0, false, NULL, &healthStr, NULL);
    if (innerJsonStr.length() > 0)
        innerJsonStr += ",";
    innerJsonStr += healthStr;
    // System information
    String idStrJSON = String::format("'$id':'%s%s%s'",
                                      pIdStr ? pIdStr : "", pIdStr ? "_" : "", System.deviceID().c_str());
    String outStr = "{" + innerJsonStr + ", " + idStrJSON +
                    (pInitialContentJsonElementList != NULL ? "," : "") +
                    (pInitialContentJsonElementList != NULL ? *pInitialContentJsonElementList : "") + "}";
    retStr = outStr.replace('\'', '\"');
}

void restAPI_QueryStatus(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    String initialContent = "'pgm': 'Shades Control'";
    restHelper_ReportHealth(NULL, &initialContent, retStr);
}

void restAPI_WipeConfig(RestAPIEndpointMsg& apiMsg, String& retStr)
{
//TODO    EEPROM.clear();
    configEEPROM.readFromEEPROM();
    Serial.println("EEPROM Cleared");
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
    restAPIEndpoints.addEndpoint("Q", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_QueryStatus, "", "");

    // Reset device
    restAPIEndpoints.addEndpoint("RESET", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_Reset, "", "");

    // Wipe config
    restAPIEndpoints.addEndpoint("WIPEALL", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_WipeConfig, "", "");

    // Help
    restAPIEndpoints.addEndpoint("HELP", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_Help, "", "");
}
