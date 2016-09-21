#ifndef PARTICLE_CLOUD_H
#define PARTICLE_CLOUD_H

typedef char* (*ParticleRxCallbackType)(char* cmdStr);

typedef char* (*ParticleStatusCallbackType)();

const int MAX_API_STR_LEN = 1000;

#include "application.h"

// Alive rate
const unsigned long ALIVE_EVENT_MILLIS = 30000;

// Status update rate
const unsigned long STATUS_UPDATE_MILLIS = 500;

static ParticleRxCallbackType __pParticleRxCallback = NULL;
static ParticleStatusCallbackType __queryStatusFn = NULL;
static char __receivedApiBuf[MAX_API_STR_LEN];
static String __appStatusStr;
static bool __particleVariablesRegistered = false;

static int __particleApiCall(String cmd)
{
    // Check if there is a callback to call
    if (__pParticleRxCallback)
    {
        cmd.toCharArray(__receivedApiBuf, MAX_API_STR_LEN);
        __pParticleRxCallback(__receivedApiBuf);
    }
    return 0;
}

class ParticleCloud
{
    public:
        // Last time an "Alive" event was sent to the particle cloud
        unsigned long _isAliveLastMillis;

        // Last time appStatusStr was updated
        unsigned long _statusUpdateLastMillis;

        ParticleCloud(ParticleRxCallbackType pParticleRxCallback, ParticleStatusCallbackType queryStatusFn)
        {
            __pParticleRxCallback = pParticleRxCallback;
            __queryStatusFn = queryStatusFn;
            _isAliveLastMillis = 0;
            _statusUpdateLastMillis = 0;
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
              if (Particle.connected())
                  Particle.publish("Shades Control Alive", __appStatusStr);
              _isAliveLastMillis = millis();
          }

          // Check if a new status check is needed
          if (Utils::isTimeout(millis(), _statusUpdateLastMillis, STATUS_UPDATE_MILLIS))
          {
              // Check for status change
              char* statusStr = __queryStatusFn();
              if (!__appStatusStr.equals(statusStr))
              {
                  Serial.println("Particle var status has changed");
                  __appStatusStr = statusStr;
              }
              _statusUpdateLastMillis = millis();
          }
        }

};

#endif
