// Notification handler
// Rob Dobson 2012-2017

#pragma once

typedef char * (*NotifyCallbackType)(const char *pNotifyIdStr,
                                     const char *initialContentJsonElementList);
typedef unsigned long (*NotifyCallbackHashType)();

const int MAX_NOTIFY_PATHS       = 10;
const int MAX_NOTIFY_STR_LEN     = 2000;
const int NOTIFY_CHECK_CHANGE_MS = 250;
const int NOTIFY_DEFAULT_PORT    = 34344; // Arbitrarily chosen
const int MAX_NOTIFY_TYPES       = 10;

class NotifyPath
{
public:
    // Can be a URL with route e.g. http://devicepilot.io:1234/device_state
    // or IP and Port eg. 192.168.0.22:1234
    String _endPoint;
    // Computed from above
    IPAddress     _ipAddrVal;
    String        _hostname;
    int           _port;
    String        _routeStr;
    bool          _updateOutstanding;
    unsigned long _timeOfLastUpdateMillis;
    int           _notifyTypeIdx;
    String        _notifyIdStr;
    String        _notifyAuthToken;
    long          _notifyRateSecs;
};

class NotifyMgr
{
public:
    enum NotifySendMethod
    {
        NOTIFY_NONE, NOTIFY_UDP, NOTIFY_POST, NOTIFY_GET
    };

public:
    NotifyPath             _notifyPaths[MAX_NOTIFY_PATHS];
    NotifyCallbackType     _notifyTypeFns[MAX_NOTIFY_TYPES];
    NotifyCallbackHashType _notifyTypeHashFns[MAX_NOTIFY_TYPES];
    NotifySendMethod       _notifyTypeSendMethods[MAX_NOTIFY_TYPES];
    unsigned long          _prevNotifyHash[MAX_NOTIFY_TYPES];
    unsigned long          _lastServiceTime;
    int _curServicePathIdx;
    UDP _udpConn;

    NotifyMgr()
    {
        _lastServiceTime   = 0;
        _curServicePathIdx = 0;
        for (int i = 0; i < MAX_NOTIFY_TYPES; i++)
        {
            _notifyTypeFns[i]         = NULL;
            _notifyTypeHashFns[i]     = NULL;
            _prevNotifyHash[i]        = 0xffffffff;
            _notifyTypeSendMethods[i] = NOTIFY_NONE;
        }
    }

    void addNotifyType(int notifyTypeIdx, NotifyCallbackType notifyCallback,
                       NotifyCallbackHashType notifyHashCallback,
                       NotifySendMethod notifySendMethod)
    {
        if ((notifyTypeIdx >= 0) && (notifyTypeIdx < MAX_NOTIFY_TYPES))
        {
            _notifyTypeFns[notifyTypeIdx]         = notifyCallback;
            _notifyTypeHashFns[notifyTypeIdx]     = notifyHashCallback;
            _notifyTypeSendMethods[notifyTypeIdx] = notifySendMethod;
        }
    }


    int getExistingEntry(String& endPoint, String& routeStr)
    {
        for (int i = 0; i < MAX_NOTIFY_PATHS; i++)
        {
            if (_notifyPaths[i]._endPoint.equalsIgnoreCase(endPoint) &&
                _notifyPaths[i]._routeStr.equalsIgnoreCase(routeStr))
            {
                return i;
            }
        }
        return -1;
    }


    int addNotifyPath(String& endPoint, int notifyTypeIdx,
                      long notifyRateSecs, String& routeStr,
                      String& notifyIdStr, String& notifyAuthToken)
    {
        // Log.trace("Adding notification %s route %s type %d secs %d",
        //             ipAndPort.c_str(), notifyRouteStr.c_str(),
        //             notifyTypeIdx, notifySecsNum);
        // Check if already present
        int pathToUse = getExistingEntry(endPoint, routeStr);

        if (pathToUse != -1)
        {
            Log.trace("addNotifyPath: %s route %s already present - amending",
                            endPoint.c_str(), routeStr.c_str());
        }
        else
        {
            // Find an empty slot
            for (int i = 0; i < MAX_NOTIFY_PATHS; i++)
            {
                if (_notifyPaths[i]._endPoint.length() == 0)
                {
                    pathToUse = i;
                    break;
                }
            }
        }
        // Add notify
        if (pathToUse == -1)
        {
            Log.trace("addNotifyPath: %s no slots left", endPoint.c_str());
            return -1;
        }

        // Split path and validate
        // Split into ipaddr and port, etc
        int colonPos = endPoint.indexOf(":");
        if (colonPos <= 0)
        {
            Log.trace("addNotifyPath: %s must contain port", endPoint.c_str());
            return -3;
        }

        // Extract the parts of the url
        String ipOrUrlBaseStr = endPoint.substring(0, colonPos);
        String portStr        = endPoint.substring(colonPos + 1);

        // Validate data for different send Methods
        if ((notifyTypeIdx < 0) || (notifyTypeIdx >= MAX_NOTIFY_TYPES))
        {
            Log.trace("addNotifyPath: notifyTypeIdx %d doesn't exist", notifyTypeIdx);
            return -4;
        }
        if (_notifyTypeSendMethods[notifyTypeIdx] == NOTIFY_NONE)
        {
            Log.trace("addNotifyPath: notifyTypeIdx %d has not been added", notifyTypeIdx);
            return -5;
        }
        switch (_notifyTypeSendMethods[notifyTypeIdx])
        {
        case NOTIFY_UDP:
            if (Utils::convIPStrToAddr(ipOrUrlBaseStr) == Utils::INADDR_NONE)
            {
                Log.trace("addNotifyPath: for UDP a valid IP Address is needed %s", endPoint.c_str());
                return -6;
            }
            break;
        }

        // Store the new notification path
        _notifyPaths[pathToUse]._endPoint               = endPoint;
        _notifyPaths[pathToUse]._hostname               = ipOrUrlBaseStr;
        _notifyPaths[pathToUse]._ipAddrVal              = Utils::convIPStrToAddr(ipOrUrlBaseStr);
        _notifyPaths[pathToUse]._port                   = portStr.toInt();
        _notifyPaths[pathToUse]._routeStr               = routeStr;
        _notifyPaths[pathToUse]._notifyTypeIdx          = notifyTypeIdx;
        _notifyPaths[pathToUse]._notifyIdStr            = notifyIdStr;
        _notifyPaths[pathToUse]._notifyAuthToken        = notifyAuthToken;
        _notifyPaths[pathToUse]._notifyRateSecs         = notifyRateSecs;
        _notifyPaths[pathToUse]._timeOfLastUpdateMillis = 0;
        _notifyPaths[pathToUse]._updateOutstanding      = true;
        Log.trace("addNotifyPath: %s added/amended in slot %d",
                        endPoint.c_str(), pathToUse);
        return pathToUse;
    }


    bool removeNotifyPath(String& endPoint, String& routeStr)
    {
        // Remove matching path
        int existing = getExistingEntry(endPoint, routeStr);

        if (existing != -1)
        {
            _notifyPaths[existing]._endPoint = "";
        }
        return existing != -1;
    }


    int sendNotificationByUDP(int pathIdx, const char *notifyStr)
    {
        _udpConn.stop();
        _udpConn.begin(_notifyPaths[pathIdx]._port);
        int rslt = _udpConn.sendPacket(notifyStr, strlen(notifyStr),
                                       _notifyPaths[pathIdx]._ipAddrVal,
                                       _notifyPaths[pathIdx]._port);
        Log.trace("Sending UDP notification to %s:%d endpoint %s rslt %d",
                        _notifyPaths[pathIdx]._hostname.c_str(),
                        _notifyPaths[pathIdx]._port,
                        _notifyPaths[pathIdx]._endPoint.c_str(),
                        rslt);
        Log.trace(notifyStr);
        if (rslt < 0)
        {
            Log.trace("Failed to send packet");
        }
        else
        {
            Log.trace("Sent notification ok");
        }
        return rslt;
    }


    int sendNotificationByPOST(int pathIdx, const char *notifyStr)
    {
        TCPClient client;

        Log.trace("Post");
        bool conn = false;
        if (_notifyPaths[pathIdx]._ipAddrVal != Utils::INADDR_NONE)
        {
            conn = client.connect(_notifyPaths[pathIdx]._ipAddrVal, _notifyPaths[pathIdx]._port);
        }
        else
        {
            conn = client.connect(_notifyPaths[pathIdx]._hostname, _notifyPaths[pathIdx]._port);
        }
        Log.trace("Connect to %s:%d%s for POST result %s",
                        _notifyPaths[pathIdx]._hostname.c_str(),
                        _notifyPaths[pathIdx]._port,
                        _notifyPaths[pathIdx]._routeStr.c_str(),
                        (conn ? "OK" : "FAIL"));
        if (conn)
        {
            String outStr = "POST /" + _notifyPaths[pathIdx]._routeStr + " HTTP/1.0";
            client.println(outStr);
            client.println("Content-Type: application/json");
            outStr = "Authorization: Token " + _notifyPaths[pathIdx]._notifyAuthToken;
            client.println(outStr);
            outStr = String::format("Content-Length: %d", strlen(notifyStr));
            client.println(outStr);
            client.println();
            client.println(notifyStr);
            Log.trace("Sent POST request");
            client.stop();
        }
    }


    int sendNotificationByGET(int pathIdx, const char *notifyStr)
    {
        TCPClient client;
        bool      conn = false;

        if (_notifyPaths[pathIdx]._ipAddrVal != Utils::INADDR_NONE)
        {
            conn = client.connect(_notifyPaths[pathIdx]._ipAddrVal, _notifyPaths[pathIdx]._port);
        }
        else
        {
            conn = client.connect(_notifyPaths[pathIdx]._hostname, _notifyPaths[pathIdx]._port);
        }
        Log.trace("Connect to %s:%d%s/%s for GET result %s",
                        _notifyPaths[pathIdx]._hostname.c_str(),
                        _notifyPaths[pathIdx]._port,
                        _notifyPaths[pathIdx]._routeStr.c_str(),
                        notifyStr,
                        (conn ? "OK" : "FAIL"));
        if (conn)
        {
            String outStr = String::format("GET /%s/%s HTTP/1.0",
                                           _notifyPaths[pathIdx]._routeStr.c_str(),
                                           notifyStr);
            client.println(outStr);
            client.println();
            Log.trace("Sent GET request");
            client.stop();
        }
    }


    void service()
    {
        // Check if a new status check is needed
        if (!Utils::isTimeout(millis(), _lastServiceTime, NOTIFY_CHECK_CHANGE_MS))
        {
            return;
        }
        _lastServiceTime = millis();
        // Keep track of notifyTypes already known to need an update
        bool notifyTypeNeedsUpdate[MAX_NOTIFY_TYPES];
        for (int j = 0; j < MAX_NOTIFY_TYPES; j++)
        {
            notifyTypeNeedsUpdate[j] = false;
        }
        // Check if any notifications registered
        for (int i = 0; i < MAX_NOTIFY_PATHS; i++)
        {
            if (_notifyPaths[i]._endPoint.length() != 0)
            {
                // Check if timed update
                if (_notifyPaths[i]._notifyRateSecs != 0)
                {
                    if (Utils::isTimeout(millis(), _notifyPaths[i]._timeOfLastUpdateMillis, abs(_notifyPaths[i]._notifyRateSecs) * 1000))
                    {
                        _notifyPaths[i]._timeOfLastUpdateMillis = millis();
                        _notifyPaths[i]._updateOutstanding      = true;
                    }
                }
                // Check if update on change
                if (_notifyPaths[i]._notifyRateSecs <= 0)
                {
                    int notifyTypeIdx = _notifyPaths[i]._notifyTypeIdx;
                    if ((notifyTypeIdx >= 0) &&
                        (notifyTypeIdx < MAX_NOTIFY_TYPES) &&
                        (_notifyTypeHashFns[notifyTypeIdx] != NULL))
                    {
                        unsigned long notifyHash = _notifyTypeHashFns[notifyTypeIdx]();
                        // Log.trace("Path %s NotifyHash %04x", _notifyPaths[i]._hostname.c_str(), notifyHash);
                        if ((_prevNotifyHash[notifyTypeIdx] != notifyHash) || (notifyTypeNeedsUpdate[notifyTypeIdx]))
                        {
                            _prevNotifyHash[notifyTypeIdx]       = notifyHash;
                            notifyTypeNeedsUpdate[notifyTypeIdx] = true;
                            _notifyPaths[i]._updateOutstanding   = true;
                            // Log.trace(" UpdateNeeded");
                        }
                        else
                        {
                            // Log.trace(" No update needed");
                        }
                    }
                }
            }
        }

        // Check status if anything requires notification
        if ((_notifyPaths[_curServicePathIdx]._endPoint.length() != 0) && (_notifyPaths[_curServicePathIdx]._updateOutstanding))
        {
            int        notifyTypeIdx = _notifyPaths[_curServicePathIdx]._notifyTypeIdx;
            const char *pIdStr       = _notifyPaths[_curServicePathIdx]._notifyIdStr.c_str();
            char       *notifyStr    =
                _notifyTypeFns[notifyTypeIdx](pIdStr, NULL);

            Log.trace("Notify to send type %d send method %d", notifyTypeIdx, _notifyTypeSendMethods[notifyTypeIdx]);
            switch (_notifyTypeSendMethods[notifyTypeIdx])
            {
            case NOTIFY_UDP:
                sendNotificationByUDP(_curServicePathIdx, notifyStr);
                break;

            case NOTIFY_POST:
                sendNotificationByPOST(_curServicePathIdx, notifyStr);
                break;

            case NOTIFY_GET:
                sendNotificationByGET(_curServicePathIdx, notifyStr);
                break;

            default:
                Log.trace("Invalid send method for notification");
                break;
            }
            _notifyPaths[_curServicePathIdx]._updateOutstanding = false;
        }
        // Find the next used path to do on the next loop
        _curServicePathIdx++;
        for ( ; _curServicePathIdx < MAX_NOTIFY_PATHS; _curServicePathIdx++)
        {
            if (_notifyPaths[_curServicePathIdx]._endPoint.length() != 0)
            {
                break;
            }
        }
        if (_curServicePathIdx >= MAX_NOTIFY_PATHS)
        {
            _curServicePathIdx = 0;
        }
    }


    unsigned long simpleHash(unsigned char *str)
    {
        unsigned long hash = 5381;
        int           c    = 0;

        while (c = *str++)
        {
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        }
        return hash;
    }
};
