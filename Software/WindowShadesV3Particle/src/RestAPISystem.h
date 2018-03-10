// REST API for System
// Rob Dobson 2012-2018

int restHelper_ReportHealth_System(int bitPosStart, unsigned long* pOutHash,
                String* pOutStr_jsonMin, String* pOutStr_urlMin)
{
    uint16_t resetReason = System.resetReason();
    uint32_t systemVersion = System.versionNumber();
    // Generate hash if required
    if (pOutHash)
    {
        unsigned long hashVal = resetReason;
        hashVal ^= systemVersion;
        *pOutHash += hashVal;
    }
    // Generate JSON string if needed
    if (pOutStr_jsonMin)
    {
        String sOut = String::format("\"rst\":\"%d\",\"ver\":\"%08x\",\"blddt\":\"%s\",\"bldtm\":\"%s\"",
                        resetReason,
                        systemVersion,
                        __DATE__, __TIME__);
        *pOutStr_jsonMin = sOut;
    }
    // Generate urlencoded string if needed
    if (pOutStr_urlMin)
    {
        String sOut = String::format("rst=%d&ver=%d",
                        resetReason,
                        systemVersion);
        *pOutStr_urlMin = sOut;
    }
    // Return number of bits in hash
    return 7;
}

// Register REST API commands
void setupRestAPI_System()
{
}
