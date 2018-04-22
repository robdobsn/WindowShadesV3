// REST API Shades Management
// Rob Dobson 2012-2018

void restAPI_ShadesControl(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    String shadeNumStr      = RestAPIEndpoints::getNthArgStr(apiMsg._pArgStr, 0);
    int    shadeNum         = shadeNumStr.toInt();
    String shadeCmdStr      = RestAPIEndpoints::getNthArgStr(apiMsg._pArgStr, 1);
    String shadeDurationStr = RestAPIEndpoints::getNthArgStr(apiMsg._pArgStr, 2);

    if ((shadeNum < 1) || (shadeNum > pWindowShades->getMaxNumShades()))
    {
        restAPI_setResultStr(retStr, false);
        return;
    }
    int shadeIdx = shadeNum - 1;
    if (pWindowShades == NULL)
    {
        restAPI_setResultStr(retStr, false);
        return;
    }
//    if (!pWindowShades->canAcceptCommand(shadeIdx, shadeCmdStr))
//    {
//        retStr = String("{ \"rslt\": \"busy\" }");
//        return;
//    }
    pWindowShades->doCommand(shadeIdx, shadeCmdStr, shadeDurationStr);
    restAPI_setResultStr(retStr, true);
}

void restAPI_ShadesConfig(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    String configStr = "{";
    // Window name
    String shadeWindowName = RestAPIEndpoints::getNthArgStr(apiMsg._pArgStr, 0);
    if (shadeWindowName.length() == 0)
        shadeWindowName = "Window Shades";
    configStr.concat("\"name\":\"");
    configStr.concat(shadeWindowName);
    configStr.concat("\"");

    // Number of shades
    String numShadesStr = RestAPIEndpoints::getNthArgStr(apiMsg._pArgStr, 1);
    int    numShades    = numShadesStr.toInt();
    if (numShades < 1)
    {
        numShades = 1;
    }
    if (numShades > pWindowShades->getMaxNumShades())
    {
        numShades = pWindowShades->getMaxNumShades();
    }
    numShadesStr = String(numShades);
    configStr.concat(",\"numShades\":\"");
    configStr.concat(numShadesStr);
    configStr.concat("\"");

    // Shade names
    for (int i = 0; i < numShades; i++)
    {
        String shadeName = RestAPIEndpoints::getNthArgStr(apiMsg._pArgStr, 2 + i);
        if (shadeName.length() == 0)
            shadeName = "Shade " + String(i+1);
        configStr.concat(",\"sh");
        configStr.concat(i);
        configStr.concat("\":\"");
        configStr.concat(shadeName);
        configStr.concat("\"");
    }
    configStr.concat("}");

    // Debug
    Log.trace(F("Writing config %s"CR), configStr.c_str());

    // Store in config
    shadesConfig.setConfigData(configStr.c_str());
    shadesConfig.writeConfig();

    // Return the query result
    String initialContent = "\"pgm\": \"Shades Control\"";
    restHelper_ReportHealth(NULL, &initialContent, retStr);
}

// Register REST API commands
void setupRestAPI_ShadesManagement()
{
    restAPIEndpoints.addEndpoint("BLIND", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_ShadesControl, "");
    restAPIEndpoints.addEndpoint("SHADE", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_ShadesControl, "");
    restAPIEndpoints.addEndpoint("SHADECFG", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_ShadesConfig, "");
}

