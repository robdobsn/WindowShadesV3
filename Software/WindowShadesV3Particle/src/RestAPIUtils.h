// Helper functions to implement general utility REST API calls
// Rob Dobson 2012-2016

void restAPI_setResultStr(String& rsltStr, bool rslt)
{
    rsltStr = String::format("{ \"rslt\": \"%s\" }", rslt? "ok" : "fail");
}

void handleReceivedApiStr(const char* requestStr, String& rsltStr)
{
    restAPIEndpoints.handleApiRequest(requestStr, rsltStr);
    if (rsltStr.length() == 0)
        restAPI_setResultStr(rsltStr, false);
}

// char *restAPI_RequestNotifications(int method, const char *cmdStr, const char *argStr, const char *msgBuffer, int msgLen,
//                    int contentLen, const unsigned char *pPayload, int payloadLen, int splitPayloadPos)
// {
//     // Get IP address and port for notifications
//     String ipAndPort = RestAPIEndpoints::getNthArgStr(argStr, 0);
//     String notifyType = RestAPIEndpoints::getNthArgStr(argStr, 1);
//     String notifySecs = RestAPIEndpoints::getNthArgStr(argStr, 2);
//     String notifyRouteStr = RestAPIEndpoints::getNthArgStr(argStr, 3);
//     String notifyIdStr = RestAPIEndpoints::getNthArgStr(argStr, 4);
//     String notifyAuthToken = RestAPIEndpoints::getNthArgStr(argStr, 5);
//     int notifyTypeIdx = notifyType.toInt();
//     int notifySecsNum = notifySecs.toInt();
//     // Register request
//     int rslt = pNotifyMgr->addNotifyPath(ipAndPort, notifyTypeIdx, notifySecsNum, notifyRouteStr, notifyIdStr, notifyAuthToken);
//     return restAPIsetResultStr(rslt != -1);
// }
