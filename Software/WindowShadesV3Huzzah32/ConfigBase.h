// Config
// Rob Dobson 2016-2018

#pragma once

#include "RdJson.h"

class ConfigBase
{
protected:
    // Data is stored in a single string as JSON
    String _dataStrJSON;

public:
    ConfigBase()
    {
    }

    ~ConfigBase()
    {
    }

    // Get config data string
    virtual const char *getConfigData()
    {
        return _dataStrJSON.c_str();
    }

    // Set the configuration data directly
    virtual void setConfigData(const char *configJSONStr)
    {
        if (strlen(configJSONStr) == 0)
            _dataStrJSON = "{}";
        else
            _dataStrJSON = configJSONStr;
    }

    virtual String getString(const char* dataPath, const char* defaultValue)
    {
        return RdJson::getString(dataPath, defaultValue, _dataStrJSON.c_str());
    }

    virtual long getLong(const char* dataPath, long defaultValue)
    {
        return RdJson::getLong(dataPath, defaultValue, _dataStrJSON.c_str());
    }

    virtual void clear()
    {
    }

    virtual bool setup()
    {
        return false;
    }

    virtual bool writeConfig()
    {
        return false;
    }

};
