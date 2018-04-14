// REST API for System
// Rob Dobson 2012-2018

int restHelper_ReportHealth_System(int bitPosStart, unsigned long* pOutHash,
                                   String* pOutStr_jsonMin, String* pOutStr_urlMin)
{
    // Generate hash if required
    if (pOutHash)
    {
        unsigned long hashVal = 0;
        *pOutHash += hashVal;
    }
    // Generate JSON string if needed
    if (pOutStr_jsonMin)
    {
        String sOut = "\"blddt\":\"" + String(__DATE__) + "\",\"bldtm\":\"" + String(__TIME__) + "\"";
        *pOutStr_jsonMin = sOut;
    }
    // Generate urlencoded string if needed
    if (pOutStr_urlMin)
    {
        String sOut = "";
        *pOutStr_urlMin = sOut;
    }
    // Return number of bits in hash
    return 7;
}

// Register REST API commands
void setupRestAPI_System()
{
}
