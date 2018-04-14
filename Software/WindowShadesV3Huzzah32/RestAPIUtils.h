// Helper functions to implement general utility REST API calls
// Rob Dobson 2012-2018

void restAPI_setResultStr(String& rsltStr, bool rslt)
{
    rsltStr = String("{ \"rslt\": \"") + (rslt ? "ok" : "fail") + String("\" }");
}

void restAPI_Reset(RestAPIEndpointMsg& apiMsg, String& retStr)
{
    // Restart the device
    ESP.restart();
}

void handleReceivedApiStr(const char* requestStr, String& rsltStr)
{
    restAPIEndpoints.handleApiRequest(requestStr, rsltStr);
    if (rsltStr.length() == 0)
        restAPI_setResultStr(rsltStr, false);
}

