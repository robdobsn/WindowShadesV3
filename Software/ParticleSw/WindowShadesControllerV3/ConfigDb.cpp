// Config DB in Particle EEPROM
// Rob Dobson 2012-2016

#include "application.h"
#include "ConfigDb.h"

#define RD_DEBUG_LEVEL 2
#define RD_DEBUG_FNAME "ConfigDb.cpp"
#include "RdDebugLevel.h"

// Configuration is held in an in-memory string called _dataStr for read operations
// When this changes it is written to EEPROM as well

// The _dataString contains the whole configuration split into numbered records
// Each record starts with a <REC_START_MARKER> character

// Overall format of the string is:
// <REC_START_MARKER>data characters<REC_SUB_STR_SEP>other data characters

// A record can be used with name:value pairs using the getRecValByName() and setRecValByName() methods

// The EEPROM can be used for several purposes and so a start position and a max length are used to
// ensure one use doesn't overwrite another
ConfigDb::ConfigDb(int eepromBaseLocation, int maxDataLen)
{
  _eepromBaseLocation = eepromBaseLocation;
  _maxDataLen = maxDataLen;
  
  // Check EEPROM has been initialised - if not just start with an empty string
  if (EEPROM.read(_eepromBaseLocation) == 0xff)
  {
    _dataStr = "";
    RD_DBG("EEPROM uninitialised, _dataStr empty");
    return;
  }

  // Find out how long the string is
  int dataStrLen = _maxDataLen-1;
  for (int chIdx = 0; chIdx < _maxDataLen; chIdx++)
  {
    if (EEPROM.read(_eepromBaseLocation+chIdx) == 0)
    {
      dataStrLen = chIdx;
      break;
    }
  }

  // Set initial size of string to avoid unnecessary resizing as we read it
  _dataStr = "";
  _dataStr.reserve(dataStrLen);

  // Fill string from EEPROM location
  for (int chIdx = 0; chIdx < dataStrLen; chIdx++)
  {
    char ch = EEPROM.read(_eepromBaseLocation+chIdx);
    _dataStr.concat(ch);
  }

  RD_DBG("Read config str: %s", _dataStr.c_str());
}

bool ConfigDb::writeToEEPROM()
{
  RD_DBG("Writing config str: %s", _dataStr.c_str());

  // Get length of string
  int dataStrLen = _dataStr.length();
  if (dataStrLen >= _maxDataLen)
    dataStrLen = _maxDataLen-1;
  
  // Write the current value of the string to EEPROM
  for (int chIdx = 0; chIdx < dataStrLen; chIdx++)
  {
    EEPROM.write(_eepromBaseLocation+chIdx, _dataStr.charAt(chIdx));
  }

  // Terminate string
  EEPROM.write(_eepromBaseLocation+dataStrLen, 0);

  return true;
}

// Get the number of records in the whole database
int ConfigDb::getNumRecs()
{
  int numRecsFound = 0;
  int lastRecPos = -1;
  for (int i = 0; i < 100000; i++)
  {
    lastRecPos = _dataStr.indexOf(REC_START_MARKER, lastRecPos+1);
    if (lastRecPos == -1)
      return numRecsFound;
    numRecsFound++;
  }
  return numRecsFound;
}

// Returns the position of the <REC_START_MARKER> of the Nth record
// or -1 if that record is not found
int ConfigDb::findRecLocation(int recIdx)
{
  int lastRecPos = -1;
  for (int i = 0; i < recIdx+1; i++)
  {
    lastRecPos = _dataStr.indexOf(REC_START_MARKER, lastRecPos+1);
    if (lastRecPos == -1)
      return -1;
  }
  return lastRecPos;
}

// Returns the number of config strings in the record given the start position
// of the record
int ConfigDb::getRecNumStrs(int startPos)
{
  int curChIdx = startPos;
  // Go through strings to find how many there are
  int numStrs = 0;
  // Iterate through chars
  while (curChIdx < (int)_dataStr.length())
  {
    char ch = _dataStr.charAt(curChIdx);
    if (ch == REC_SUB_STR_SEP)
    {
      numStrs++;
    }
    else if (((ch == REC_START_MARKER) && (curChIdx != startPos)) || (ch == 0))
    {
      break;
    }
    curChIdx++;
  }
  return numStrs;
}

// Extract Nth string from the record starting at position startPos
int ConfigDb::extractStrN(int startPos, int strIdx, String& rtnStr)
{
  rtnStr = "";
  int curChIdx = startPos;
  // Go through strings to find the one we want
  bool bEndOfRec = false;
  for (int i = 0; i < strIdx+1; i++)
  {
    // Iterate through chars
    while (curChIdx < (int)_dataStr.length())
    {
      char ch = _dataStr.charAt(curChIdx);
      if ((ch == REC_START_MARKER) && (curChIdx == startPos))
      {
        curChIdx++;
        continue;
      }
      if (ch == REC_SUB_STR_SEP)
        break;
      if ((ch == REC_START_MARKER) || (ch == 0))
      {
        bEndOfRec = true;
        break;
      }
      if (i == strIdx)
        rtnStr.concat(ch);
      curChIdx++;
    }
    if ((bEndOfRec) || (curChIdx == (int)_dataStr.length()))
      break;
    // Skip the terminator
    curChIdx++;
  }
  return curChIdx;
}

// Get a string from a record by index of the string in the record
bool ConfigDb::getRecStrByIdx(int recIdx, int strIdx, String& recStr)
{
  // Find location of record
  int recPos = findRecLocation(recIdx);
  if (recPos < 0)
    return false;
  // Get the string
  extractStrN(recPos, strIdx, recStr);
  return true;
}

// Extract a value 
int ConfigDb::extractValByName(int startPos, String valName, String& rtnStr)
{
  rtnStr = "";
  int curChIdx = startPos;
  // Go through strings to find the one we want
  String nameStr = "";
  bool bInNamePart = true;
  bool bFoundNameMatch = false;
  while (curChIdx < (int)_dataStr.length())
  {
    char ch = _dataStr.charAt(curChIdx);
    if ((ch == REC_START_MARKER) && (curChIdx == startPos))
    {
      curChIdx++;
      continue;
    }
    if ((ch == REC_START_MARKER) || (ch == 0))
    {
      if (!bInNamePart && bFoundNameMatch)
        return true;
      break;
    }
    if (ch == REC_SUB_STR_SEP)
    {
      if (!bInNamePart && bFoundNameMatch)
        return true;
      bInNamePart = true;
      bFoundNameMatch = false;
      nameStr = "";
    }
    else
    {
      if (bInNamePart)
      {
        if (ch == ':')
        {
          bInNamePart = false;
          if (nameStr.trim().equals(valName.trim()))
            bFoundNameMatch = true;
        }
        else
        {
          nameStr.concat(ch);
        }
      }
      else if (bFoundNameMatch)
      {
        rtnStr.concat(ch);
      }
    }
    curChIdx++;
  }
  return false;
}

bool ConfigDb::getRecValByName(int recIdx, String nameStr, String& valStr)
{
  // Find location of record
  int recPos = findRecLocation(recIdx);
  if (recPos < 0)
    return false;
  // Extract value
  extractValByName(recPos, nameStr, valStr);
  return true;
}

void ConfigDb::setPosValByName(int recIdx, int startPos, String nameStr, String& valStr)
{
  String newRecStr;
  recStrBegin(newRecStr);
  // Get number of strings in record
  int numStrs = getRecNumStrs(startPos);
  RD_DBG("setValByName: _dataStr=%s", _dataStr.c_str());
  RD_DBG("setValByName: numStrs = %d, startPos = %d", numStrs, startPos);
  // Go through the strings
  for (int i = 0; i < numStrs; i++)
  {
    bool bMatch = false;
    String curStr;
    extractStrN(startPos, i, curStr);
    int colonPos = curStr.indexOf(':');
    if (colonPos > 0)
    {
      String curNameStr = curStr.substring(0, colonPos);
      if (curNameStr.trim().equals(nameStr.trim()))
      {
        bMatch = true;
      }
    }
    // If no match then add this to replacement record
    if (!bMatch)
    {
      recStrAdd(newRecStr, curStr);
      RD_DBG("setValByName: addStr %s", curStr.c_str());
    }
  }
  // Add the new name/value pair
  String newNVPair = nameStr;
  newNVPair.concat(":");
  newNVPair.concat(valStr);
  // Add to record
  recStrAdd(newRecStr, newNVPair);
  // Done
  recStrDone(newRecStr);
  // Change / update record
  changeOrAppendRec(recIdx, newRecStr);
}

bool ConfigDb::setRecValByName(int recIdx, String nameStr, String& valStr)
{
  // Find location of record
  int recPos = findRecLocation(recIdx);
  if (recPos < 0)
  {
    // Append as must be new record
    recIdx = -1;
    recPos = _dataStr.length();
    RD_DBG("setRecValByName, appending");
  }
  // Set value now we know the position of the record
  setPosValByName(recIdx, recPos, nameStr, valStr);
  return true;
}

// Helper functions to create a new record
bool ConfigDb::recStrBegin(String& recStr)
{
  recStr = "";
  recStr.concat(REC_START_MARKER);
  return true;
}

bool ConfigDb::recStrAdd(String& recStr, String strToAdd)
{
  recStr.concat(strToAdd);
  recStr.concat(REC_SUB_STR_SEP);
  return true;
}

bool ConfigDb::recStrDone(String& recStr)
{
  return true;
}

// Pass -1 to append
bool ConfigDb::changeOrAppendRec(int recIdx, String& newRec)
{
  RD_DBG("changeOrAppendRec %d", recIdx);
  
  // Check for append
  bool bAppend = false;
  int locPos = _dataStr.length();
  if (recIdx == -1)
  {
    bAppend = true;
    RD_DBG("changeOrAppendRec: append True");
  }
  else
  {
    // Find the location of the record
    locPos = findRecLocation(recIdx);
    if (locPos < 0)
    {
      RD_DBG("changeOrAppendRec: failed to find rec");
      return false;
    }
  }
  
  // Get the _dataStr up to this point
  String headStr = _dataStr.substring(0, locPos);
  RD_DBG("changeOrAppendRec: found rec at %d, headStr=%s", locPos, headStr.c_str());

  // Get the _dataStr after the changing record
  String tailStr = "";
  
  // Find the location of the next record
  if (!bAppend)
  {
    locPos = findRecLocation(recIdx+1);
    if (locPos > 0)
    {
      tailStr = _dataStr.substring(locPos);
    }
  }
    
  // Reconstruct the final string
  _dataStr = headStr + newRec + tailStr;
  RD_DBG("changeOrAppendRec: new _dataStr=%s", _dataStr.c_str());

  // Write back to non-volatile
  writeToEEPROM();

  // Ok
  return true;
}


