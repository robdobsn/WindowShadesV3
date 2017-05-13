// Web Server
// Rob Dobson 2012-2017

#pragma once

#include "RdWebServerResources.h"
#include "RestAPIEndpoints.h"

class RdWebServer
{
private:
    // Max length of an http request
    static const int HTTPD_MAX_REQ_LENGTH = 2048;

    // Max length of response header
    static const int HTTPD_MAX_HDR_LENGTH = 2000;

    // Each call to service() process max this number of chars received from a TCP connection
    static const int MAX_CHS_IN_SERVICE_LOOP = 200;

    // Max len of HTTP endpoint and arg strings
    static const int MAX_ENDPOINT_LEN = 100;
    static const int MAX_ARGSTR_LEN   = 100;

    // Timeouts
    static const unsigned long MAX_MS_IN_CLIENT_STATE_WITHOUT_DATA = 2000;

private:
    // Port
    int _TCPPort;

    // TCP server and client
    TCPServer *_pTCPServer;
    TCPClient _TCPClient;

    // Last time web server actively responded to a request
    unsigned long _lastWebServerResponseMs;

    // Possible states of web server
public:
    enum WebServerState
    {
        WEB_SERVER_OFFLINE, WEB_SERVER_WAIT_CONNECT, WEB_SERVER_CONNECTED_BUT_NO_CLIENT, WEB_SERVER_HAS_CLIENT
    };

private:
    // Current web-server state
    WebServerState _webServerState;
    unsigned long  _webServerStateEntryMs;

    // HTTP Request buffer - single-threaded server so create here to avoid unnecessary heap alloc
    // or stack overflow
    int  _httpReqBufPos      = 0;
    int  _charsOnHttpReqLine = 0;
    char _httpReqBuf[HTTPD_MAX_REQ_LENGTH + 1];

    // Process HTTP Request
    bool handleReceivedHttp(char *httpReq, int httpReqLen, TCPClient& tcpClient);

    // Extract endpoint arguments
    bool extractEndpointArgs(char *buf, char *pEndpointStr, int maxEndpointStrLen, char *pArgStr, int maxArgStrLen, int& contentLen);

    // REST API Endpoints
    RestAPIEndpoints *_pRestAPIEndpoints;

    // Web server resources
    RdWebServerResourceDescr *_pWebServerResources;
    int _numWebServerResources;

    // HTTP response header - see comment above about single threaded web server
    char _httpRespHeader[HTTPD_MAX_HDR_LENGTH + 1];
    void formHTTPResponse(char *pRespHeader, const char *rsltCode, const char *contentType, const char *respStr, int contentLen);

    // Utility
    static void formStringFromCharBuf(String& outStr, char *pStr, int len);
    void setState(WebServerState newState);

public:
    RdWebServer();
    virtual ~RdWebServer();

    void start(int port);
    void stop();
    void restart();
    void service();
    const char *connStateStr();
    char connStateChar();

    int connState()
    {
        return _webServerState;
    }


    // Add endpoints to the web server
    void addRestAPIEndpoints(RestAPIEndpoints *pRestAPIEndpoints);

    // Add resources to the web server
    void addStaticResources(RdWebServerResourceDescr *pResources, int numResources);

    // Get last time web server responded to a request
    unsigned long getLastWebResponseMs()
    {
        return _lastWebServerResponseMs;
    }


    // Methods
    static const int METHOD_OTHER   = 0;
    static const int METHOD_GET     = 1;
    static const int METHOD_POST    = 2;
    static const int METHOD_OPTIONS = 3;

    // Helpers
    static unsigned char *getPayloadDataFromMsg(char *msgBuf, int msgLen, int& payloadLen);
    static int getContentLengthFromMsg(char *msgBuf);
};
