// NotifyManager
// Rob Dobson 2016-2017

#pragma once

typedef char * (*QueryStatusCallbackType)();

const int MAX_NOTIFY_PATHS       = 10;
const int MAX_NOTIFY_STR_LEN     = 2000;
const int NOTIFY_CHECK_CHANGE_MS = 500;
const int NOTIFY_DEFAULT_PORT    = 34344; // Arbitrarily chosen

class NotifyPath
{
public:
    String    _ipAddrStr;
    IPAddress _ipAddrVal;
    int       _port;
    bool      _updateOutstanding;
};

class NotifyMgr
{
public:
    NotifyPath              _notifyPaths[MAX_NOTIFY_PATHS];
    QueryStatusCallbackType _queryStatusFn;
    char          _prevNotifyStr[MAX_NOTIFY_STR_LEN + 1];
    unsigned long _lastServiceTime;
    int           _curServicePathIdx;
    UDP           _udpConn;

    NotifyMgr(QueryStatusCallbackType queryStatusFn)
    {
        _queryStatusFn     = queryStatusFn;
        _prevNotifyStr[0]  = 0;
        _lastServiceTime   = 0;
        _curServicePathIdx = 0;
    }
    int getExistingEntry(String& ipAddr)
    {
        for (int i = 0; i < MAX_NOTIFY_PATHS; i++)
        {
            if (_notifyPaths[i]._ipAddrStr.equalsIgnoreCase(ipAddr))
            {
                return i;
            }
        }
        return -1;
    }


    int addNotifyPath(String& ipAddr)
    {
        // Check if already present
        if (getExistingEntry(ipAddr) != -1)
        {
            Log.trace("addNotifyPath: %s already present", ipAddr.c_str());
            return -2;
        }
        // Find an empty slot
        int pathToUse = -1;
        for (int i = 0; i < MAX_NOTIFY_PATHS; i++)
        {
            if (_notifyPaths[i]._ipAddrStr.length() == 0)
            {
                pathToUse = i;
                break;
            }
        }
        if (pathToUse != -1)
        {
            _notifyPaths[pathToUse]._ipAddrStr = ipAddr;
            _notifyPaths[pathToUse]._port      = NOTIFY_DEFAULT_PORT;
            // Split into ipaddr and port
            int    colonPos = ipAddr.indexOf(":");
            String ipStr    = ipAddr;
            if (colonPos > 0)
            {
                _notifyPaths[pathToUse]._port = ipAddr.substring(colonPos + 1).toInt();
                ipStr = ipAddr.substring(0, colonPos);
            }
            _notifyPaths[pathToUse]._ipAddrVal         = WiFiConn::convIPStrToAddr(ipStr);
            _notifyPaths[pathToUse]._updateOutstanding = true;
            Log.trace("addNotifyPath: %s port %d added at %d", ipAddr, _notifyPaths[pathToUse]._port, pathToUse);
            return pathToUse;
        }
        Log.trace("addNotifyPath: %s no slots left", ipAddr);
        return -1;
    }


    bool removeNotifyPath(String& ipAddr)
    {
        // Remove matching path
        int existing = getExistingEntry(ipAddr);

        if (existing != -1)
        {
            _notifyPaths[existing]._ipAddrStr = "";
        }
        return existing != -1;
    }


    void service()
    {
        // Check if a new status check is needed
        if (!Utils::isTimeout(millis(), _lastServiceTime, NOTIFY_CHECK_CHANGE_MS))
        {
            return;
        }
        _lastServiceTime = millis();
        // Check if any notifications registered
        int countOfUsedPaths = 0;
        for (int i = 0; i < MAX_NOTIFY_PATHS; i++)
        {
            if (_notifyPaths[i]._ipAddrStr.length() != 0)
            {
                countOfUsedPaths++;
            }
        }
        if (countOfUsedPaths == 0)
        {
            return;
        }
        // Check for status change
        char *statusStr = _queryStatusFn();
        if (strcmp(_prevNotifyStr, statusStr) != 0)
        {
            Log.trace("Status has changed %s", statusStr);
            // Set the update required flag on each entry
            for (int i = 0; i < MAX_NOTIFY_PATHS; i++)
            {
                if (_notifyPaths[i]._ipAddrStr.length() != 0)
                {
                    _notifyPaths[i]._updateOutstanding = true;
                }
            }
        }
        strncpy(_prevNotifyStr, statusStr, MAX_NOTIFY_STR_LEN);
        _prevNotifyStr[MAX_NOTIFY_STR_LEN] = 0;
        // Check status if anything requires notification
        if ((_notifyPaths[_curServicePathIdx]._ipAddrStr.length() != 0) && (_notifyPaths[_curServicePathIdx]._updateOutstanding))
        {
            _udpConn.stop();
            _udpConn.begin(_notifyPaths[_curServicePathIdx]._port);
            int rslt = _udpConn.sendPacket(statusStr, strlen(statusStr), _notifyPaths[_curServicePathIdx]._ipAddrVal, _notifyPaths[_curServicePathIdx]._port);
            if (rslt < 0)
            {
                Log.trace("Failed to send packet");
            }
            else
            {
                Log.trace("Sent notification ok");
            }
            _notifyPaths[_curServicePathIdx]._updateOutstanding = false;
        }
        // Find the next used path to do on the next loop
        _curServicePathIdx++;
        for ( ; _curServicePathIdx < MAX_NOTIFY_PATHS; _curServicePathIdx++)
        {
            if (_notifyPaths[_curServicePathIdx]._ipAddrStr.length() != 0)
            {
                break;
            }
        }
        if (_curServicePathIdx >= MAX_NOTIFY_PATHS)
        {
            _curServicePathIdx = 0;
        }
    }
};
