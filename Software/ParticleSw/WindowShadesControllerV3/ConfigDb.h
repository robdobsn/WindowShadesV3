// Config DB in Particle EEPROM
// Rob Dobson 2012-2016

#ifndef _CONFIG_DB_H_
#define _CONFIG_DB_H_

class ConfigDb
{
  private:
    // Data is stored in a single string representation
    String _dataStr;

    // Base location to store dataStr in EEPROM
    int _eepromBaseLocation;

    // Max length of data
    int _maxDataLen;

    // Write to non-volatile storage
    bool writeToEEPROM();

    // Record start marker
    const char REC_START_MARKER = '\x01';

    // Record sub-string separator
    const char REC_SUB_STR_SEP = '\x02';

    // Helper functions
    int findRecLocation(int userRecIdx);
    int extractStrN(int startPos, int strIdx, String& rtnStr);
    String formUserRec(String userEnableCode, String userName, String userPin, String userCardNo);
    bool changeUserRecord(int userIdx, String& userEnableCode, String& userName, String& userPin, String& userCardNo);
    int extractValByName(int startPos, String valName, String& rtnStr);
    void setPosValByName(int recIdx, int startPos, String nameStr, String& valStr);

  public:
    ConfigDb(int eepromBaseLocation, int maxDataLen);
    void readFromEEPROM();
    int getNumRecs();
    int getRecNumStrs(int startPos);
    bool getRecStrByIdx(int recIdx, int strIdx, String& recStr);
    bool recStrBegin(String& recStr);
    bool recStrAdd(String& recStr, String strToAdd);
    bool recStrDone(String& recStr);
    bool getRecValByName(int recIdx, String nameStr, String& valStr);
    bool setRecValByName(int recIdx, String nameStr, String& valStr);
  
    // Pass -1 to append
    bool changeOrAppendRec(int recIdx, String& newRec);
};

#endif
