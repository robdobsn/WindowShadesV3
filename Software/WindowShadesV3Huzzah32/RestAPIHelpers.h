// Helper functions to implement REST API calls
// Rob Dobson 2012-2018

int restHelper_ReportHealth_Shades(int bitPosStart, bool incRelockInStr,
                                   unsigned long* pOutHash, String* pOutStr_jsonMin,
                                   String* pOutStr_urlMin)
{
    // Get information on status
    String shadeWindowName = shadesConfig.getString("name", "");
    if (shadeWindowName.length() == 0)
        shadeWindowName = "Window Shades";
    Log.notice(F("Shades name %s"CR), shadeWindowName.c_str());
    int numShades= shadesConfig.getLong("numShades", 0);
    if (numShades < 1)
    {
        numShades = 1;
    }
    if (numShades > pWindowShades->getMaxNumShades())
    {
        numShades = pWindowShades->getMaxNumShades();
    }
    String numShadesStr = String(numShades);

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
            lightSensorStr.concat(String(i));
            lightSensorStr.concat("\",\"v\":\"");
            lightSensorStr.concat(String(analogRead(lightSensorPins[i])));
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
            String shadeName = shadesConfig.getString(("sh" + String(i)).c_str(), "");
            if (shadeName.length() == 0)
                shadeName = "Shade " + String(i+1);
            shadesDetailStr.concat("{\"name\": \"");
            shadesDetailStr.concat(shadeName);
            shadesDetailStr.concat("\", \"num\": \"");
            shadesDetailStr.concat(String(i + 1));
            bool isBusy = pWindowShades->isBusy(i);
            shadesDetailStr.concat("\", \"busy\": \"");
            shadesDetailStr.concat(String(isBusy ? "1" : "0"));
            shadesDetailStr.concat("\"}");
            if (i != numShades - 1)
            {
                shadesDetailStr.concat(",");
            }
        }
        shadesDetailStr.concat("]");
        // Shade name and number
        String shadesStr = "\"numShades\":\"" + numShadesStr + "\",\"name\":\"" + shadeWindowName + "\"";
        // Compile output string
        String sOut = shadesStr + "," + shadesDetailStr + "," + lightSensorStr;
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
    // Log.notice(F("RepHealthHash %ld"CR), hashVal);
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
    String healthStrShades;
    hashUsedBits += restHelper_ReportHealth_Shades(0, false, NULL, &healthStrShades, NULL);
    if (innerJsonStr.length() > 0)
        innerJsonStr += ",";
    innerJsonStr += healthStrShades;
    // System information
    retStr = "{" + innerJsonStr + (pIdStr ? "," + String(pIdStr) : String("")) +
                    (pInitialContentJsonElementList != NULL ? "," : "") +
                    (pInitialContentJsonElementList != NULL ? *pInitialContentJsonElementList : "") + "}";
}

void restAPI_QueryStatus(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    String initialContent = "\"pgm\": \"Shades Control\"";
    restHelper_ReportHealth(NULL, &initialContent, retStr);
}

void restAPI_WipeConfig(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    shadesConfig.clear();
    Log.notice(F("restAPI: Shades config cleared"CR));
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
