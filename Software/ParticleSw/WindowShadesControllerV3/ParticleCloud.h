// ParticleCloud
// Rob Dobson 2016-2017

#pragma once

#include "application.h"
#include "Utils.h"

// Time between time re-syncs on the particle cloud
const int CLOUD_RESYNC_TIME_SECS = 24 * 60 * 60;

// Alive rate
const unsigned long ALIVE_EVENT_MILLIS = 30000;

// Status update rate
const unsigned long STATUS_UPDATE_MILLIS = 5000;

// Callback functions
typedef char * (*ParticleRxCallbackType)(const char *cmdStr);
static ParticleRxCallbackType __pParticleRxCallback = NULL;
typedef char * (*ParticleStatusCallbackType)();
static ParticleStatusCallbackType __queryStatusFn = NULL;

// Statics
static String __appStatusStr;
static bool   __particleVariablesRegistered = false;

// Static function used as a callback
static int __particleApiCall(String cmd)
{
    // Check if there is a callback to call
    if (__pParticleRxCallback)
    {
        __pParticleRxCallback(cmd.c_str());
    }
    return 0;
}


// Class for handling particle cloud
// Handles cloud functions, variables, updates, etc
class ParticleCloud
{
public:
    ParticleCloud(ParticleRxCallbackType pParticleRxCallback, ParticleStatusCallbackType queryStatusFn)
    {
        __pParticleRxCallback   = pParticleRxCallback;
        __queryStatusFn         = queryStatusFn;
        _isAliveLastMillis      = 0;
        _statusUpdateLastMillis = 0;
        _bCloudSyncTime         = true;
        _lastCloudTimeSync      = 0;
    }

    void RegisterVariables()
    {
        // Variable for application status
        Particle.variable("status", __appStatusStr);

        // Function for API Access
        Particle.function("apiCall", __particleApiCall);
    }


    void Service(const bool particleCloudProcessRequired)
    {
        // Give the particle system time
        if (particleCloudProcessRequired && Particle.connected())
        {
            Particle.process();
        }

        // Check if particle variables registered
        if (!__particleVariablesRegistered)
        {
            if (Particle.connected())
            {
                RegisterVariables();
                __particleVariablesRegistered = true;
            }
        }

        // Say we're alive to the particle cloud every now and then
        if (Utils::isTimeout(millis(), _isAliveLastMillis, ALIVE_EVENT_MILLIS))
        {
            if (Particle.connected())
            {
                Particle.publish("Shades Control Alive", __appStatusStr);
            }
            _isAliveLastMillis = millis();
        }

        // Check if a new status check is needed
        if (Utils::isTimeout(millis(), _statusUpdateLastMillis, STATUS_UPDATE_MILLIS))
        {
            // Check for status change
            char *statusStr = __queryStatusFn();
            if (!__appStatusStr.equals(statusStr))
            {
                Log.trace("Particle var status has changed");
                Log.trace(statusStr);
                __appStatusStr = statusStr;
            }
            _statusUpdateLastMillis = millis();
        }

        if (Particle.connected())
        {
            // Check if particle time needs to be updated
            if (Utils::isTimeout(millis(), _lastCloudTimeSync, CLOUD_RESYNC_TIME_SECS * 1000))
            {
                _bCloudSyncTime    = true;
                _lastCloudTimeSync = millis();
            }

            // Sync time if required
            if (_bCloudSyncTime)
            {
                Particle.syncTime();
                _bCloudSyncTime = false;
            }
        }
    }


    char *localIPStr()
    {
        IPAddress ipA = WiFi.localIP();

        sprintf(_localIPStr, "%d.%d.%d.%d", ipA[0], ipA[1], ipA[2], ipA[3]);
        return _localIPStr;
    }

    const char* BSSIDStr()
    {
        byte bssid[6];
        WiFi.BSSID(bssid);
        sprintf(_BSSIDStr, "%02X:%02X:%02X:%02X:%02X:%02X", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
        return _BSSIDStr;
    }

    const char* MACAddrStr()
    {
        byte macaddr[6];
        WiFi.macAddress(macaddr);
        sprintf(_MACAddrStr, "%02X:%02X:%02X:%02X:%02X:%02X", macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
        return _MACAddrStr;
    }    

    const char* connStateStr()
    {
        if (WiFi.listening())
            return "ListenMode";
        if (Particle.connected())
            return "CloudConn";
        if (WiFi.ready())
            return "WiFiReady";
        if (WiFi.connecting())
            return "Connecting";
        return "None";
    }

private:
    // Last time an "Alive" event was sent to the particle cloud
    unsigned long _isAliveLastMillis;

    // Last time appStatusStr was updated
    unsigned long _statusUpdateLastMillis;

    // Cloud time sync handlers
    bool          _bCloudSyncTime;
    unsigned long _lastCloudTimeSync;

    // Local IP Addr as string
    char _localIPStr[20];
    // BSSID as string
    char _BSSIDStr[40];
    // MAC as string
    char _MACAddrStr[20];
};
