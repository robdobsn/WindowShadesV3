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

// char *restAPI_RequestNotifications(int method, char *cmdStr, char *argStr, char *msgBuffer, int msgLen,
//                                    int contentLen, unsigned char *pPayload, int payloadLen, int splitPayloadPos)
// {
//     // Get IP address and port for notifications
//     String ipAndPort = RdWebServer::getNthArgStr(argStr, 0);
//     // Register request
//     int rslt = pNotifyMgr->addNotifyPath(ipAndPort);
//
//     return setResultStr(rslt != -1);
// }
//
//
// char *restAPI_WipeConfig(int method, char *cmdStr, char *argStr, char *msgBuffer, int msgLen,
//                          int contentLen, unsigned char *pPayload, int payloadLen, int splitPayloadPos)
// {
//     EEPROM.clear();
//     pConfigDb->readFromEEPROM();
//     Serial.println("EEPROM Cleared");
//     return setResultStr(true);
// }

char *handleReceivedApiStr(const char *requestStr)
{
    char* rsltStr = restAPIEndpoints.handleApiRequest(requestStr);
    if (strlen(rsltStr) == 0)
        return restAPIsetResultStr(false);
    return rsltStr;
}
