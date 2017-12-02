// Helper functions to implement general utility REST API calls
// Rob Dobson 2012-2016

// Rest API Implementations
const int MAX_REST_API_RETURN_LEN = 1000;
static char restAPIHelpersBuffer[MAX_REST_API_RETURN_LEN];
char* restAPIsetResultStr(bool rslt)
{
    sprintf(restAPIHelpersBuffer, "{ \"rslt\": \"%s\" }", rslt? "ok" : "fail");
    return restAPIHelpersBuffer;
}

char *restAPI_RequestNotifications(int method, const char *cmdStr, const char *argStr, const char *msgBuffer, int msgLen,
                   int contentLen, const unsigned char *pPayload, int payloadLen, int splitPayloadPos)
{
    // Get IP address and port for notifications
    String ipAndPort = RestAPIEndpoints::getNthArgStr(argStr, 0);
    String notifyType = RestAPIEndpoints::getNthArgStr(argStr, 1);
    String notifySecs = RestAPIEndpoints::getNthArgStr(argStr, 2);
    String notifyRouteStr = RestAPIEndpoints::getNthArgStr(argStr, 3);
    String notifyIdStr = RestAPIEndpoints::getNthArgStr(argStr, 4);
    String notifyAuthToken = RestAPIEndpoints::getNthArgStr(argStr, 5);
    int notifyTypeIdx = notifyType.toInt();
    int notifySecsNum = notifySecs.toInt();
    // Register request
    int rslt = pNotifyMgr->addNotifyPath(ipAndPort, notifyTypeIdx, notifySecsNum, notifyRouteStr, notifyIdStr, notifyAuthToken);
    return restAPIsetResultStr(rslt != -1);
}


char *restAPI_WipeConfig(int method, const char *cmdStr, const char *argStr, const char *msgBuffer, int msgLen,
                   int contentLen, const unsigned char *pPayload, int payloadLen, int splitPayloadPos)
{
    EEPROM.clear();
    configEEPROM.readFromEEPROM();
    Serial.println("EEPROM Cleared");
    return restAPIsetResultStr(true);
}

char* restAPI_Reset(int method, const char *cmdStr, const char *argStr, const char *msgBuffer, int msgLen,
                   int contentLen, const unsigned char *pPayload, int payloadLen, int splitPayloadPos)
{
    Log.info("RestAPI: Reset Requested");
    // Set a flag to indicate reset is pending
    __systemResetPendingTimeMs = millis();
    // Respond positively
    return restAPIsetResultStr(true);
}

char *handleReceivedApiStr(const char *requestStr)
{
    char* rsltStr = restAPIEndpoints.handleApiRequest(requestStr);
    if (strlen(rsltStr) == 0)
        return restAPIsetResultStr(false);
    return rsltStr;
}
