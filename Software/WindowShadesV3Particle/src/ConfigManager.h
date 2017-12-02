// Config
// Rob Dobson 2016-2017

#pragma once

#include "RdJson.h"

class ConfigManager
{
private:
    // Data is stored in a single string as JSON
    String _dataStrJSON;

public:
    ConfigManager()
    {
    }

    ~ConfigManager()
    {
    }

    // Get config data string
    const char *getConfigData()
    {
        return _dataStrJSON.c_str();
    }

    // Set the configuration data directly
    void setConfigData(const char *configJSONStr)
    {
        if (strlen(configJSONStr) == 0)
            _dataStrJSON = "{}";
        else
            _dataStrJSON = configJSONStr;
    }

    String getString(const char* dataPath, const char* defaultValue)
    {
        return RdJson::getString(dataPath, defaultValue, _dataStrJSON);
    }

    long getLong(const char* dataPath, long defaultValue)
    {
        return RdJson::getLong(dataPath, defaultValue, _dataStrJSON);
    }

};
