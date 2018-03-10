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
#include "RdJson.h"

// Alive rate and minimum time between status change cloud publishes
const unsigned long ALIVE_EVENT_MILLIS = 30000;
const unsigned long             CLOUD_STATUS_MIN_BETWEEN_MS = 1000;

static ParticleRxCallbackType __pParticleRxCallback = NULL;
static ParticleCallbackType __queryStatusFn = NULL;
static ParticleHashCallbackType __queryStatusHashFn = NULL;
static char __receivedApiBuf[MAX_API_STR_LEN];
static String __appStatusStr;
static unsigned long __lastStatusHashValue = 0xffffffff;
static bool   __particleVariablesRegistered = false;
static String                   __appEventStr;

// Static function used as a callback
static int __particleApiCall(String cmd)
{
    // Check if there is a callback to call
    if (__pParticleRxCallback)
    {
        cmd.toCharArray(__receivedApiBuf, MAX_API_STR_LEN);
        String retStr;
        __pParticleRxCallback(__receivedApiBuf, retStr);
    String rslt = RdJson::getString("rslt", "UKN", retStr.c_str());
    if (rslt == "ok")
      return 0;   // OK
    if (rslt == "fail")
      return -1;  // Fail
    if (rslt == "UKN")
      return -3;  // No rslt in response JSON
    return -4;    // Other result
    }
  return -2;      // Nothing to call
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

  void registerVariables()
    {
        // Variable for application status
        Particle.variable("status", __appStatusStr);

    // Variable for application event
    Particle.variable("event", __appEventStr);

        // Function for API Access
    Particle.function("exec", __particleApiCall);
    }

  void service()
        {
        // Check if particle variables registered
        if (!__particleVariablesRegistered)
        {
      if (WiFi.ready())
      {
            if (Particle.connected())
            {
          Log.trace("Registering particle variables");
          registerVariables();
                __particleVariablesRegistered = true;
            }
        }
    }

        // Say we're alive to the particle cloud every now and then
    if (WiFi.ready())
        {
            if (Particle.connected())
            {
        bool reportNeeded = Utils::isTimeout(millis(), _isAliveLastMillis, ALIVE_EVENT_MILLIS);
        if (!reportNeeded)
        {
          unsigned long statusHash = __queryStatusHashFn();
          if ((statusHash != __lastStatusHashValue) && (Utils::isTimeout(millis(), _isAliveLastMillis, CLOUD_STATUS_MIN_BETWEEN_MS)))
            reportNeeded = true;
        }
        if (reportNeeded)
        {
          String ip = WiFi.localIP();
          Log.info("Publishing status to Particle Cloud IP %s Millis %lu", ip.c_str(), millis());
                Particle.publish(_systemName.c_str(), __appStatusStr);
          Log.info("Published status to Particle Cloud at millis %lu", _isAliveLastMillis);
          _isAliveLastMillis = millis();
        }
            }
        }

        // Check if a new status check is needed
        if (Utils::isTimeout(millis(), _statusUpdateLastMillis, _statusEventCheckMs))
        {
      if (WiFi.ready())
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
      }
            _statusUpdateLastMillis = millis();
        }
            }

  void recordEvent(String& eventStr)
  {
    if (WiFi.ready())
    {
      if (Particle.connected())
      {
        __appEventStr = eventStr;
        Particle.publish(_systemName.c_str(), __appEventStr);
      }
    }
    Log.info("Published event to Particle Cloud %lu", millis());
  }
};
