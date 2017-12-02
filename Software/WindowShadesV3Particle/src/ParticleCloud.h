// Particle cloud handler
// Rob Dobson 2012-2017

#pragma once

typedef void (*ParticleRxCallbackType)(const char* cmdStr, String& retStr);

typedef void (*ParticleCallbackType)(const char* idStr,
                String* initialContentJsonElementList, String& retStr);
typedef unsigned long (*ParticleHashCallbackType)();

const int MAX_API_STR_LEN = 1000;

#include "application.h"
#include "Utils.h"

// Alive rate
const unsigned long ALIVE_EVENT_MILLIS = 30000;

static ParticleRxCallbackType __pParticleRxCallback = NULL;
static ParticleCallbackType __queryStatusFn = NULL;
static ParticleHashCallbackType __queryStatusHashFn = NULL;
static char __receivedApiBuf[MAX_API_STR_LEN];
static String __appStatusStr;
static unsigned long __lastStatusHashValue = 0xffffffff;
static bool   __particleVariablesRegistered = false;

// Static function used as a callback
static int __particleApiCall(String cmd)
{
    // Check if there is a callback to call
    if (__pParticleRxCallback)
    {
        cmd.toCharArray(__receivedApiBuf, MAX_API_STR_LEN);
        String retStr;
        __pParticleRxCallback(__receivedApiBuf, retStr);
    }
    return 0;
}


// Class for handling particle cloud
// Handles cloud functions, variables, updates, etc
class ParticleCloud
{
public:
    static const unsigned long HASH_VALUE_FOR_ALWAYS_REPORT = 0xffffffff;

    // Last time an "Alive" event was sent to the particle cloud
    unsigned long _isAliveLastMillis;

    // Last time appStatusStr was updated
    unsigned long _statusUpdateLastMillis;

    // Time between update checks
    unsigned long _statusEventCheckMs;

    // System name
    String _systemName;

    ParticleCloud(ParticleRxCallbackType pParticleRxCallback,
        ParticleCallbackType queryStatusFn,
        ParticleHashCallbackType queryStatusHashFn,
        unsigned long statusEventCheckMs,
        const char* systemName)
    {
        __pParticleRxCallback   = pParticleRxCallback;
        __queryStatusFn         = queryStatusFn;
        __queryStatusHashFn = queryStatusHashFn;
        _isAliveLastMillis      = 0;
        _statusUpdateLastMillis = 0;
        _statusEventCheckMs = statusEventCheckMs;
        _systemName = systemName;
    }

    void RegisterVariables()
    {
        // Variable for application status
        Particle.variable("status", __appStatusStr);

        // Function for API Access
        Particle.function("apiCall", __particleApiCall);
    }

    void Service()
        {
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
            // Log.info("Publishing Event to Particle Cloud");
            if (Particle.connected())
            {
                Particle.publish(_systemName.c_str(), __appStatusStr);
            }
            _isAliveLastMillis = millis();
            // Log.info("Published Event to Particle Cloud");
        }

        // Check if a new status check is needed
        if (Utils::isTimeout(millis(), _statusUpdateLastMillis, _statusEventCheckMs))
        {
            // Check for status change
            unsigned long statusHash = __queryStatusHashFn();
            if ((statusHash != __lastStatusHashValue) || (statusHash == HASH_VALUE_FOR_ALWAYS_REPORT))
            {
                // Log.info("Particle var status has changed");
                __queryStatusFn(NULL, NULL, __appStatusStr);
                __lastStatusHashValue = statusHash;
                // Log.info("Particle var status updated ok");
            }
            _statusUpdateLastMillis = millis();
        }
            }

};
