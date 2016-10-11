// WiFi connection handler
// Rob Dobson 2012-2016

// Amended 2016-10-10
// Improved handling of situations where:
//   WiFi is offline when the device starts up - previously device would enter "smart setup" state and not connect
//   when WiFi did eventually come online
//   Also now cycles power on WiFi completely after a number of unsuccessful attempts

#ifndef _WIFI_CONN_HEADER_
#define _WIFI_CONN_HEADER_

#include "string.h"
#include "Utils.h"

const int WIFI_CONN_TIMEOUT_SECS = 30;
const int WIFI_RECONN_TIMEOUT_SECS = 3;
const int WIFI_IP_TIMEOUT_SECS = 60;
const int CLOUD_RESYNC_TIME_SECS = 24 * 60 * 60;
const int WIFI_CONN_RECONNS_BEFORE_POWER_CYCLE = 120;
const int WIFI_CONN_MIN_CONNECTED_SECS = 10;

#define PARTICLE_CLOUD_CONNECT 1

#define WIFI_DEBUG_SHOW 1

#ifdef WIFI_DEBUG_SHOW
#define WIFI_DEBUG(x, ...) Serial.printlnf("[WIFI_DEBUG: %s:%d]" x, "WiFiConn.h", __LINE__, ##__VA_ARGS__);
#else
#define WIFI_DEBUG(x, ...)
#endif

class WiFiConn
{
public:
    enum ConnStates { CONN_STATE_NONE, CONN_STATE_POWER_CYCLE,
        CONN_STATE_CONNECTING, CONN_STATE_WAIT_FOR_IP,
        CONN_STATE_WIFI_CONNECTED, CONN_STATE_CLOUD_CONNECTED };

    static const unsigned long INADDR_NONE = ((unsigned long) 0xffffffff);

  private:
    // Local IP Addr as string
    char _localIPStr[20];
    // BSSID as string
    char _BSSIDStr[20];
    // MAC as string
    char _MACAddrStr[20];
    // States
    ConnStates _connState;
    // Time current state entered
    unsigned long _stateEntryMillis;
    int _reconnCount;
    // Config data
    String _ssid;
    String _password;
    String _method;
    String _ipAddress;
    String _wifiSubnetMask;
    String _wifiGatewayIP;
    String _wifiDNSIP;
    // Cloud handlers
    bool _bCloudSyncTime;
    unsigned long _lastCloudTimeSync;

  public:
    WiFiConn()
    {
      _stateEntryMillis = 0;
      _connState = CONN_STATE_NONE;
      _bCloudSyncTime = true;
      _lastCloudTimeSync = 0;
        _reconnCount = 0;
    }

    void setState(ConnStates connState)
    {
      _connState = connState;
      _stateEntryMillis = millis();
        WIFI_DEBUG("WiFiState %s", connStateStr());
    }

    void setupWifi(bool clearCredentialsFirst)
    {
        if (clearCredentialsFirst)
        WiFi.clearCredentials();
        String debugStr = "setupWiFi ";
      // Connection method
        if (strcasecmp(_method, "WEP") == 0)
      {
        WiFi.setCredentials(_ssid, _password, WEP);
        debugStr += _ssid + " WEP";
      }
        else if (strcasecmp(_method, "WPA") == 0)
      {
        WiFi.setCredentials(_ssid, _password, WPA);
        debugStr += _ssid + " WPA";
      }
        else if (strcasecmp(_method, "WPA2") == 0)
      {
        WiFi.setCredentials(_ssid, _password, WPA2);
        debugStr += _ssid + " WPA2";
      }
      else
      {
        WiFi.setCredentials(_ssid);
        debugStr += _ssid + " OPEN";
      }
        unsigned long ipAddr = WiFiConn::convIPStrToAddr(_ipAddress);
        if (ipAddr == INADDR_NONE)
      {
        debugStr += " DynamicIP";
        WiFi.useDynamicIP();
      }
      else
      {
        unsigned long ipMask = WiFiConn::convIPStrToAddr(_wifiSubnetMask);
        unsigned long ipGate = WiFiConn::convIPStrToAddr(_wifiGatewayIP);
        unsigned long ipDNS = WiFiConn::convIPStrToAddr(_wifiDNSIP);
        WiFi.setStaticIP(IPAddress(ipAddr), IPAddress(ipMask), IPAddress(ipGate), IPAddress(ipDNS));
        WiFi.useStaticIP();
        debugStr += " FixedIP " + IPAddress(ipAddr);
        debugStr += " Mask " + IPAddress(ipMask);
        debugStr += " Gateway " + IPAddress(ipGate);
        debugStr += " DNS " + IPAddress(ipDNS);
      }
        WIFI_DEBUG("%s", debugStr.c_str());
    }

    void initialConnect()
    {
        // Turn on wifi
        WiFi.on();
        setupWifi(true);
      WiFi.connect();
      setState(CONN_STATE_CONNECTING);      
    }
    
    void setConnectParams(String& ssid, String& password, String& method, String& ipAddress, String& wifiSubnetMask, String& wifiGatewayIP, String& wifiDNSIP)
    {
      _ssid = ssid;
      _password = password;
      _method = method;
      _ipAddress = ipAddress;
      _wifiSubnetMask = wifiSubnetMask;
      _wifiGatewayIP = wifiGatewayIP;
      _wifiDNSIP = wifiDNSIP;
    }

    void reconnectStart()
    {
        setupWifi(false);
        WiFi.listen(false);
        WiFi.connect();
        _reconnCount++;
        setState(CONN_STATE_CONNECTING);
    }

    void powerCycleStart()
    {
        setupWifi(true);
        WiFi.off();
        setState(CONN_STATE_POWER_CYCLE);
    }

    void powerCycleEnd()
    {
        WiFi.on();
        WiFi.connect();
        setState(CONN_STATE_CONNECTING);
    }

    void changeConnectParams(String& ssid, String& password, String& method, String& ipAddress, String& wifiSubnetMask, String& wifiGatewayIP, String& wifiDNSIP)
    {
        setConnectParams(ssid, password, method, ipAddress, wifiSubnetMask, wifiGatewayIP, wifiDNSIP);
        setupWifi(true);
        WiFi.off();
        delay(1000);
        WiFi.connect();
        setState(CONN_STATE_CONNECTING);
    }

    void service()
    {
      switch(_connState)
      {
            case CONN_STATE_POWER_CYCLE:
            {
                if (Utils::isTimeout(millis(), _stateEntryMillis, WIFI_RECONN_TIMEOUT_SECS * 1000))
                {
                    WIFI_DEBUG("Power Cycle - turning on");
                    powerCycleEnd();
                }
                break;
            }
        case CONN_STATE_CONNECTING:
        {
          if (WiFi.ready() && (strlen(WiFi.SSID()) != 0))
          {
                    WIFI_DEBUG("On SSID %s, waiting for IP address", WiFi.SSID());
            setState(CONN_STATE_WAIT_FOR_IP);
          }
          else if (Utils::isTimeout(millis(), _stateEntryMillis, WIFI_CONN_TIMEOUT_SECS * 1000))
          {
                    WIFI_DEBUG("Timed out connecting - retry listening");
                    if (_reconnCount > WIFI_CONN_RECONNS_BEFORE_POWER_CYCLE)
                    {
                        _reconnCount = 0;
                        powerCycleStart();
                        break;
                    }
                    reconnectStart();
          }
          break;
        }
        case CONN_STATE_WAIT_FOR_IP:
        {
          IPAddress localIP = WiFi.localIP();
                if (WiFi.ready())
          {
                    WIFI_DEBUG("WiFiConnected %s", localIPStr());
            setState(CONN_STATE_WIFI_CONNECTED);  

            // Cloud connect if required
            #ifdef PARTICLE_CLOUD_CONNECT
            Particle.connect();
                    WIFI_DEBUG("Connecting to particle cloud");
            #endif
            
          }
          else if (Utils::isTimeout(millis(), _stateEntryMillis, WIFI_IP_TIMEOUT_SECS * 1000))
          {
                    WIFI_DEBUG("Timed out waiting for IP address");
            setState(CONN_STATE_CONNECTING);
          }
          break;
        }
        case CONN_STATE_WIFI_CONNECTED:
        case CONN_STATE_CLOUD_CONNECTED:
        {
          IPAddress localIP = WiFi.localIP();
                if (Utils::isTimeout(millis(), _stateEntryMillis, WIFI_CONN_MIN_CONNECTED_SECS * 1000))
                {
                    if (!WiFi.ready())
          {
            setState(CONN_STATE_CONNECTING);
                        WIFI_DEBUG("Connection lost - WiFi.Ready %d", WiFi.ready());
            break;
          }
                }

          // Check for connection to cloud
          #ifdef PARTICLE_CLOUD_CONNECT
          if (Particle.connected())
          {
            if (_connState == CONN_STATE_WIFI_CONNECTED)
            {
              WIFI_DEBUG("Connected to Cloud");
                        setState(CONN_STATE_CLOUD_CONNECTED);
            }
            
            // Give the particle system time
            Particle.process();
            
            // Check if particle time needs to be updated
            if (Utils::isTimeout(millis(), _lastCloudTimeSync, CLOUD_RESYNC_TIME_SECS * 1000))
            {
              _bCloudSyncTime = true;
              _lastCloudTimeSync = millis();
            }

            // Sync time if required
            if (_bCloudSyncTime)
            {
              Particle.syncTime();
              _bCloudSyncTime = false;
            }
            
          }
          else
          {
            if (_connState == CONN_STATE_CLOUD_CONNECTED)
            {
                        if (Utils::isTimeout(millis(), _stateEntryMillis, WIFI_CONN_MIN_CONNECTED_SECS * 1000))
                        {
              WIFI_DEBUG("Lost connection to Cloud");
                            setState(CONN_STATE_WIFI_CONNECTED);
                        }
            }
          }

          #endif
          break;
        }
      }
    }

    const char* connStateStr()
    {
      switch(_connState)
      {
        case CONN_STATE_NONE:
          return "None";
            case CONN_STATE_POWER_CYCLE:
            return "PowerCycle";
        case CONN_STATE_CONNECTING:
          return "WaitConn";
        case CONN_STATE_WAIT_FOR_IP:
          return "WaitIP";
        case CONN_STATE_WIFI_CONNECTED:
          return "WiFiConnected";
        case CONN_STATE_CLOUD_CONNECTED:
          return "CloudConnected";
      }
      return "Unknown";
    }
    
    ConnStates getConnState()
    {
        return _connState;
    }

    const char connStateChar()
    {
        switch(_connState)
        {
            case CONN_STATE_NONE:
            return 'N';
            case CONN_STATE_POWER_CYCLE:
            return '0';
            case CONN_STATE_CONNECTING:
            return 'W';
            case CONN_STATE_WAIT_FOR_IP:
            return 'I';
            case CONN_STATE_WIFI_CONNECTED:
            return 'C';
            case CONN_STATE_CLOUD_CONNECTED:
            return 'P';
        }
        return 'K';
    }

    bool isConnected()
    {
        return _connState == CONN_STATE_WIFI_CONNECTED || _connState == CONN_STATE_CLOUD_CONNECTED;
    }

    IPAddress localIP()
    {
      return WiFi.localIP();
    }
    
    char* localIPStr()
    {
      IPAddress ipA = WiFi.localIP();
      sprintf(_localIPStr, "%d.%d.%d.%d", ipA[0], ipA[1], ipA[2], ipA[3]);
      return _localIPStr;
    }

    const char* SSID()
    {
      return WiFi.SSID();
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

    long RSSI()
    {
      return WiFi.RSSI();
    }

    // Following code from Unix sources
    static unsigned long convIPStrToAddr(String& inStr)
    {
      char buf[30];
      char* cp = buf;
      inStr.toCharArray(cp, 29);
      unsigned long val, base, n;
      char c;
      unsigned long parts[4], *pp = parts;
    
      for (;;) {
        /*
         * Collect number up to ``.''.
         * Values are specified as for C:
         * 0x=hex, 0=octal, other=decimal.
         */
        val = 0; base = 10;
        if (*cp == '0') {
          if (*++cp == 'x' || *cp == 'X')
            base = 16, cp++;
          else
            base = 8;
        }
        while ((c = *cp) != '\0') {
          if (isascii(c) && isdigit(c)) {
            val = (val * base) + (c - '0');
            cp++;
            continue;
          }
          if (base == 16 && isascii(c) && isxdigit(c)) {
            val = (val << 4) + 
              (c + 10 - (islower(c) ? 'a' : 'A'));
            cp++;
            continue;
          }
          break;
        }
        if (*cp == '.') {
          /*
           * Internet format:
           *  a.b.c.d
           *  a.b.c (with c treated as 16-bits)
           *  a.b (with b treated as 24 bits)
           */
          if (pp >= parts + 3 || val > 0xff)
            return (INADDR_NONE);
          *pp++ = val, cp++;
        } else
          break;
      }
      /*
       * Check for trailing characters.
       */
      if (*cp && (!isascii(*cp) || !isspace(*cp)))
        return (INADDR_NONE);
      /*
       * Concoct the address according to
       * the number of parts specified.
       */
      n = pp - parts + 1;
      switch (n) {
    
      case 1:       /* a -- 32 bits */
        break;
    
      case 2:       /* a.b -- 8.24 bits */
        if (val > 0xffffff)
          return (INADDR_NONE);
        val |= parts[0] << 24;
        break;
    
      case 3:       /* a.b.c -- 8.8.16 bits */
        if (val > 0xffff)
          return (INADDR_NONE);
        val |= (parts[0] << 24) | (parts[1] << 16);
        break;
    
      case 4:       /* a.b.c.d -- 8.8.8.8 bits */
        if (val > 0xff)
          return (INADDR_NONE);
        val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
        break;
      }
      return (val);
    }
};

#endif
