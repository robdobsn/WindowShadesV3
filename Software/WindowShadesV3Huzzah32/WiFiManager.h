// WiFi Manager
// Rob Dobson 2018

#pragma once

#include <WiFi.h>
#include <WString.h>
#include "ConfigNVS.h"

class WiFiManager
{
private:
    String _ssid;
    String _password;
    String _hostname;
    String _defaultHostname;
    unsigned long _lastWifiBeginAttemptMs;
    bool _wifiFirstBeginDone;
    static constexpr unsigned long TIME_BETWEEN_WIFI_BEGIN_ATTEMPTS_MS = 30000;
    ConfigBase* _pConfigBase;

public:
    WiFiManager()
    {
        _lastWifiBeginAttemptMs = 0;
        _wifiFirstBeginDone = false;
        _pConfigBase = NULL;
    }

    void setup(ConfigBase* pSysConfig, const char* defaultHostname)
    {
        _pConfigBase = pSysConfig;
        _defaultHostname = defaultHostname;
        // Get the SSID, password and hostname if available
        _ssid = pSysConfig->getString("WiFiSSID", "");
        _password = pSysConfig->getString("WiFiPW", "");
        _hostname = pSysConfig->getString("WiFiHostname", _defaultHostname.c_str());
    }

    void service()
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            if ((!_wifiFirstBeginDone) || (Utils::isTimeout(millis(), _lastWifiBeginAttemptMs, TIME_BETWEEN_WIFI_BEGIN_ATTEMPTS_MS)))
            {
                _wifiFirstBeginDone = true;
                WiFi.begin(_ssid.c_str(), _password.c_str());
                WiFi.setHostname(_hostname.c_str());
                _lastWifiBeginAttemptMs = millis();
                Log.notice(F("WiFiManager: WiFi not connected - WiFi.begin with SSID %s"CR), _ssid.c_str());
            }
        }
    }

    bool isConnected()
    {
        return (WiFi.status() == WL_CONNECTED);
    }

    String formConfigStr()
    {
        return "{\"WiFiSSID\":\"" + _ssid + "\",\"WiFiPW\":\"" + _password + "\",\"WiFiHostname\":\"" + _hostname + "\"}";
    }
    void setCredentials(String& ssid, String& pw, String& hostname)
    {
        _ssid = ssid;
        _password = pw;
        if (_hostname.length() > 0)
            _hostname = hostname;
        if (_pConfigBase)
        {
            _pConfigBase->setConfigData(formConfigStr().c_str());
            _pConfigBase->writeConfig();
        }
        // Disconnect so re-connection with new credentials occurs
        WiFi.disconnect();
    }

    void clearCredentials()
    {
        _ssid = "";
        _password = "";
        _hostname = _defaultHostname;
        if (_pConfigBase)
        {
            _pConfigBase->setConfigData(formConfigStr().c_str());
            _pConfigBase->writeConfig();
        }
    }
};

