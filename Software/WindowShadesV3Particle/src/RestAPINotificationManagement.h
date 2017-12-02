// REST API for Notification Management
// Rob Dobson 2012-2017

void restAPI_RequestNotifications(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    // Get IP address and port for notifications
    String ipAndPort = RestAPIEndpoints::getNthArgStr(apiMsg._pArgStr, 0);
    String notifyType = RestAPIEndpoints::getNthArgStr(apiMsg._pArgStr, 1);
    String notifySecs = RestAPIEndpoints::getNthArgStr(apiMsg._pArgStr, 2);
    String notifyRouteStr = RestAPIEndpoints::getNthArgStr(apiMsg._pArgStr, 3);
    String notifyIdStr = RestAPIEndpoints::getNthArgStr(apiMsg._pArgStr, 4);
    String notifyAuthToken = RestAPIEndpoints::getNthArgStr(apiMsg._pArgStr, 5);
    int notifyTypeIdx = notifyType.toInt();
    int notifySecsNum = notifySecs.toInt();
    // Register request
    int rslt = pNotifyMgr->addNotifyPath(ipAndPort, notifyTypeIdx, notifySecsNum, notifyRouteStr, notifyIdStr, notifyAuthToken);
    restAPI_setResultStr(retStr, rslt != -1);
}

// Register REST API commands
void setupRestAPI_NotificationManager()
{
    // Add notifications
      restAPIEndpoints.addEndpoint("NO", RestAPIEndpointDef::ENDPOINT_CALLBACK, restAPI_RequestNotifications, "");
}
