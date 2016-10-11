// Helper functions to implement general utility REST API calls
// Rob Dobson 2012-2016

char* restAPI_RequestNotifications(int method, char*cmdStr, char* argStr, char* msgBuffer, int msgLen,
                int contentLen, unsigned char* pPayload, int payloadLen, int splitPayloadPos)
{
  // Get IP address and port for notifications
  String ipAndPort = RdWebServer::getNthArgStr(argStr, 0);
  // Register request
  int rslt = pNotifyMgr->addNotifyPath(ipAndPort);
  return setResultStr(rslt != -1);
}

char* restAPI_WipeConfig(int method, char*cmdStr, char* argStr, char* msgBuffer, int msgLen,
                int contentLen, unsigned char* pPayload, int payloadLen, int splitPayloadPos)
{
    EEPROM.clear();
    pConfigDb->readFromEEPROM();
    Serial.println("EEPROM Cleared");
    return setResultStr(true);
}

char* handleReceivedApiStr(char*cmdStr)
{
    if (!pWebServer)
    return setResultStr(false);

    // Get the command
    static char* emptyStr = "";
    String restCmd = RdWebServer::getNthArgStr(cmdStr, 0).toUpperCase();
    char* argStart = strstr(cmdStr, "/");
    if (argStart == NULL)
    argStart = emptyStr;
    else
    argStart++;
    Serial.printlnf("CMD: %s", cmdStr);
    // Check against valid commands
    int numWebCmds = pWebServer->getNumWebCommands();
    for (int i = 0; i < numWebCmds; i++)
    {
     CmdCallbackType callback;
     int cmdType;
     char* pWebCmdStr = pWebServer->getNthWebCmd(i, cmdType, callback);
     if (!pWebCmdStr)
         continue;
     if (cmdType != RdWebServerCmdDef::CMD_CALLBACK)
         continue;
     if (restCmd.equalsIgnoreCase(pWebCmdStr))
     {
         return callback(0, NULL, argStart, NULL, 0, 0, NULL, 0, 0);
     }
    }
    return setResultStr(false);
}
