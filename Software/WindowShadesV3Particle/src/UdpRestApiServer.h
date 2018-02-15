// UDP REST API Server
// Rob Dobson 2012-2016

#pragma once

// Callback when data received
typedef void (*UdpReceivedDataCallbackType)(const char*cmdStr, String& retStr);
static UdpReceivedDataCallbackType __pRxCallback = NULL;

class UdpRestApiServer
{
private:
    // Max length of a request
    static const int UDPREST_MAX_REQ_LENGTH = 256;

    // Port
    int _udpPort;

    // UDP
    UDP _udpSocket;

    // Read buffer
    char _udpReadBuffer[UDPREST_MAX_REQ_LENGTH+1];

    // connection state
    bool _isConnected = false;

public:
    UdpRestApiServer(UdpReceivedDataCallbackType pRxCallback)
    {
        __pRxCallback = pRxCallback;
    }
    void start(int port)
    {
        _udpPort = port;
        _isConnected = false;
    }
    void stop()
    {
        _udpSocket.stop();
    }
    void service()
    {
        if (!_isConnected)
        {
            if (WiFi.ready())
            {
                _isConnected = true;
                _udpSocket.begin(_udpPort);
                Log.info("Started UDP listening on port %d", _udpPort);
            }
            return;
        }
        else
        {
            if (!WiFi.ready())
            {
                _isConnected = false;
                _udpSocket.stop();
                return;
            }
        }

        // Handle any received packets
        int packetSize = _udpSocket.parsePacket();
        if (packetSize)
        {
            // Default response
            String retStr = "401";
            // Store sender ip and port
            IPAddress remoteIpAddr = _udpSocket.remoteIP();
            uint16_t remotePort = _udpSocket.remotePort();
            // Get data
            int chCount = _udpSocket.read(_udpReadBuffer, UDPREST_MAX_REQ_LENGTH);
            if ((chCount > 0) && (chCount <= UDPREST_MAX_REQ_LENGTH))
            {
                _udpReadBuffer[chCount] = '\0';
                __pRxCallback(_udpReadBuffer, retStr);
            }
            _udpSocket.sendPacket(retStr.c_str(), retStr.length(),
                        remoteIpAddr, remotePort);
        }
    }
};
