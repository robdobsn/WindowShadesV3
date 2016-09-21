#ifndef _WIFI_CONN_HEADER_
#define _WIFI_CONN_HEADER_

#include "Utils.h"

const int WIFI_CONN_TIMEOUT_SECS = 20;
const int WIFI_IP_TIMEOUT_SECS = 60;
const int CLOUD_RESYNC_TIME_SECS = 24 * 60 * 60;

#define PARTICLE_CLOUD_CONNECT 1

#define WIFI_DEBUG_SHOW 1

#ifdef WIFI_DEBUG_SHOW
#define WIFI_DEBUG(x, ...) Serial.printlnf("[WIFI_DEBUG: %s:%d]" x, "WiFiConn.h", __LINE__, ##__VA_ARGS__);
#else
#define WIFI_DEBUG(x, ...)
#endif

class WiFiConn
{
  private:
    // Local IP Addr as string
    char _localIPStr[20];
    // BSSID as string
    char _BSSIDStr[20];
    // MAC as string
    char _MACAddrStr[20];
    // States
    enum ConnStates { CONN_STATE_NONE, CONN_STATE_CONNECTING, CONN_STATE_WAIT_FOR_IP, CONN_STATE_WIFI_CONNECTED, CONN_STATE_CLOUD_CONNECTED };
    ConnStates _connState;
    // Time current state entered
    unsigned long _stateEntryMillis;
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
    }

    void setState(ConnStates connState)
    {
      _connState = connState;
      _stateEntryMillis = millis();
    }

    void doConnect()
    {
      // Turn off and on wifi
      WiFi.off();
      WiFi.on();
      String debugStr = "WiFi::doConnect ";
      // Connection method
      if (strcmp(_method, "WEP") == 0)
      {
        WiFi.setCredentials(_ssid, _password, WEP);
        debugStr += _ssid + " WEP";
      }
      else if (strcmp(_method, "WPA") == 0)
      {
        WiFi.setCredentials(_ssid, _password, WPA);
        debugStr += _ssid + " WPA";
      }
      else if (strcmp(_method, "WPA2") == 0)
      {
        WiFi.setCredentials(_ssid, _password, WPA2);
        debugStr += _ssid + " WPA2";
      }
      else
      {
        WiFi.setCredentials(_ssid);
        debugStr += _ssid + " OPEN";
      }
      if ((_ipAddress.length() == 0) || (!isdigit(_ipAddress.charAt(0))))
      {
        debugStr += " DynamicIP";
        WiFi.useDynamicIP();
      }
      else
      {
        unsigned long ipAddr = WiFiConn::convIPStrToAddr(_ipAddress);
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
      Serial.println(debugStr);
      WiFi.connect();
      setState(CONN_STATE_CONNECTING);      

      // Debug
      WIFI_DEBUG("WiFi connecting");
    }
    
    void connect(String& ssid, String& password, String& method, String& ipAddress, String& wifiSubnetMask, String& wifiGatewayIP, String& wifiDNSIP)
    {
      _ssid = ssid;
      _password = password;
      _method = method;
      _ipAddress = ipAddress;
      _wifiSubnetMask = wifiSubnetMask;
      _wifiGatewayIP = wifiGatewayIP;
      _wifiDNSIP = wifiDNSIP;
      doConnect();
    }

    void service()
    {
      switch(_connState)
      {
        case CONN_STATE_CONNECTING:
        {
          if (WiFi.ready() && (strlen(WiFi.SSID()) != 0))
          {
            setState(CONN_STATE_WAIT_FOR_IP);
            WIFI_DEBUG("On SSID %s, waiting for IP address", WiFi.SSID());
          }
          else if (Utils::isTimeout(millis(), _stateEntryMillis, WIFI_CONN_TIMEOUT_SECS * 1000))
          {
            WiFi.connect();
            setState(CONN_STATE_CONNECTING);
            WIFI_DEBUG("Timed out connecting");
          }
          break;
        }
        case CONN_STATE_WAIT_FOR_IP:
        {
          IPAddress localIP = WiFi.localIP();
          if (localIP[0] != 0)
          {
            setState(CONN_STATE_WIFI_CONNECTED);  
            WIFI_DEBUG("WiFiConnected %s", localIPStr());

            // Cloud connect if required
            #ifdef PARTICLE_CLOUD_CONNECT
            Particle.connect();
            #endif
            
          }
          else if (Utils::isTimeout(millis(), _stateEntryMillis, WIFI_IP_TIMEOUT_SECS * 1000))
          {
            doConnect();
            setState(CONN_STATE_CONNECTING);
            WIFI_DEBUG("Timed out waiting for IP address");
          }
          break;
        }
        case CONN_STATE_WIFI_CONNECTED:
        case CONN_STATE_CLOUD_CONNECTED:
        {
          IPAddress localIP = WiFi.localIP();
          if ((!WiFi.ready()) || (localIP[0] == 0))
          {
            WiFi.disconnect();
            doConnect();
            setState(CONN_STATE_CONNECTING);
            WIFI_DEBUG("Connection lost");
            break;
          }

          // Check for connection to cloud
          #ifdef PARTICLE_CLOUD_CONNECT
          if (Particle.connected())
          {
            if (_connState == CONN_STATE_WIFI_CONNECTED)
            {
              WIFI_DEBUG("Connected to Cloud");
              _connState = CONN_STATE_CLOUD_CONNECTED;
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
              WIFI_DEBUG("Lost connection to Cloud");
              _connState = CONN_STATE_WIFI_CONNECTED;
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
    
    bool isConnected()
    {
      if (WiFi.connecting())
        return false;
      IPAddress localIP = WiFi.localIP();
      return (localIP[0] != 0);
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

    #define INADDR_NONE ((unsigned long) 0xffffffff)

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
