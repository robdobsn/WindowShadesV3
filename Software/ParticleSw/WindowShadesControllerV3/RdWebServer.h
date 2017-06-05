// Web Server
// Rob Dobson 2012-2017

#pragma once

#include "RdWebServerResources.h"
#include "RestAPIEndpoints.h"

class RdWebServer;

class RdWebClient
{
private:
    // Max length of an http request
    static const int HTTPD_MAX_REQ_LENGTH = 2048;

    // Each call to service() process max this number of chars received from a TCP connection
    static const int MAX_CHS_IN_SERVICE_LOOP = 500;

    // Timeouts
    static const unsigned long MAX_MS_IN_CLIENT_STATE_WITHOUT_DATA = 2000;

    // TCP client
    TCPClient _TCPClient;

public:
    RdWebClient();

    void service(RdWebServer *pWebServer);

    enum WebClientState
    {
        WEB_CLIENT_NONE, WEB_CLIENT_ACCEPTED
    };

    void setState(WebClientState newState);
    const char *connStateStr();
    WebClientState clientConnState()
    {
        return _webClientState;
    }

private:
    // Current client state
    WebClientState _webClientState;
    unsigned long  _webClientStateEntryMs;

    // HTTP Request
    String _httpReqStr;
};

class RdWebServer
{
public:
    RdWebServer();
    virtual ~RdWebServer();

    void start(int port);
    void stop();
    void service();
    const char *connStateStr();
    char connStateChar();

    int serverConnState()
    {
        return _webServerState;
    }


    int clientConnections()
    {
        int connCount = 0;
        for (int clientIdx = 0; clientIdx < MAX_WEB_CLIENTS; clientIdx++)
        {
            if (_webClients[clientIdx].clientConnState() == RdWebClient::WebClientState::WEB_CLIENT_ACCEPTED)
            {
                connCount++;
            }
        }
        return connCount;
    }


    // Add endpoints to the web server
    void addRestAPIEndpoints(RestAPIEndpoints *pRestAPIEndpoints);

    // Add resources to the web server
    void addStaticResources(RdWebServerResourceDescr *pResources, int numResources);

public:
    // Get an available client
    TCPClient available();

    // Process HTTP Request
    bool handleReceivedHttp(const char *httpReq, int httpReqLen, TCPClient& tcpClient);

    // Methods
    static const int METHOD_OTHER   = 0;
    static const int METHOD_GET     = 1;
    static const int METHOD_POST    = 2;
    static const int METHOD_OPTIONS = 3;

private:
    // Clients
    static const int MAX_WEB_CLIENTS = 5;

private:
    // Port
    int _TCPPort;

    // TCP server
    TCPServer *_pTCPServer;

    // Client
    RdWebClient _webClients[MAX_WEB_CLIENTS];

    // Max chunk size of HTTP response sending
    static const int HTTPD_MAX_RESP_CHUNK_SIZE = 500;

    // Possible states of web server
public:
    enum WebServerState
    {
        WEB_SERVER_STOPPED, WEB_SERVER_WAIT_CONN, WEB_SERVER_BEGUN
    };

private:
    // Current web-server state
    WebServerState _webServerState;
    unsigned long  _webServerStateEntryMs;

    // Extract endpoint arguments
    bool extractEndpointArgs(const char *buf, String& endpointStr, String& argStr, int& contentLen);

    // REST API Endpoints
    RestAPIEndpoints *_pRestAPIEndpoints;

    // Web server resources
    RdWebServerResourceDescr *_pWebServerResources;
    int _numWebServerResources;

    // Utility
    static void formStringFromCharBuf(String& outStr, char *pStr, int len);
    void setState(WebServerState newState);

    // Form HTTP response
    void formHTTPResponse(String& respStr, const char *rsltCode,
                          const char *contentType, const char *respBody, int contentLen);

private:
    // Helpers
    static unsigned char *getPayloadDataFromMsg(const char *msgBuf, int msgLen, int& payloadLen);
    static int getContentLengthFromMsg(const char *msgBuf);
};
