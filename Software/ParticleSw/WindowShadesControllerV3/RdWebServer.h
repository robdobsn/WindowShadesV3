// Web Server
// Rob Dobson 2012-2016

#ifndef _RD_WEB_SERVER_
#define _RD_WEB_SERVER_

#include "RdWebServerResources.h"

class WiFiConn;

typedef char* (*CmdCallbackType)(int method, char*cmdStr, char* argStr, char* msgBuffer, int msgLen,
                int contentLen, unsigned char* pPayload, int payloadLen, int splitPayloadPos);

class RdWebServerCmdDef
{
    public:
        static const int CMD_CALLBACK = 1;
        RdWebServerCmdDef(char* pStr, int cmdType, CmdCallbackType callback)
        {
            int stlen = strlen(pStr);
            _pCmdStr = new char[stlen+1];
            strcpy(_pCmdStr, pStr);
            _cmdType = cmdType;
            _callback = callback;
        };
        ~RdWebServerCmdDef()
        {
            delete _pCmdStr;
        }
        char* _pCmdStr;
        int _cmdType;
        CmdCallbackType _callback;
};


class RdWebServer
{
    private:
        // Max length of an http request
        static const int HTTPD_MAX_REQ_LENGTH = 2048;
        
        // Max length of response header
        static const int HTTPD_MAX_HDR_LENGTH = 2000;
        
        // Each call to service() process max this number of chars received from a TCP connection
        static const int MAX_CHS_IN_SERVICE_LOOP = 200;
        
        // Max len of HTTP cmd and arg strings
        static const int MAX_CMDSTR_LEN = 100;
        static const int MAX_ARGSTR_LEN = 100;
        
        // Max commands server can accommodate
        static const int MAX_WEB_SERVER_CMDS = 50;

        // Timeouts
        static const unsigned long MAX_MS_IN_CLIENT_STATE_WITHOUT_DATA = 2000;

  private:
    // Port
    int _TCPPort;

    // WiFi connection
    WiFiConn* _pWiFiConn;

    // TCP server and client
    TCPServer* _pTCPServer;
    TCPClient _TCPClient;

    // State of web server
  public:
    enum WebServerState { WEB_SERVER_OFFLINE, WEB_SERVER_WAIT_CONNECT, WEB_SERVER_CONNECTED_BUT_NO_CLIENT, WEB_SERVER_HAS_CLIENT };
  private:
    WebServerState _webServerState;
    unsigned long _webServerStateEntryMs;

    // HTTP Request buffer - single-threaded server so create here to avoid unnecessary heap alloc
    // or stack overflow
    int _httpReqBufPos = 0;
    int _charsOnHttpReqLine = 0;
    char _httpReqBuf[HTTPD_MAX_REQ_LENGTH+1];

    // Process HTTP Request
    bool handleReceivedHttp(char* httpReq, int httpReqLen, TCPClient& tcpClient);

    // Extract command arguments
    bool extractCmdArgs(char* buf, char* pCmdStr, int maxCmdStrLen, char* pArgStr, int maxArgStrLen, int& contentLen);

    // Web server commands
    RdWebServerCmdDef* _pWebServerCmds[MAX_WEB_SERVER_CMDS];
    int _numWebServerCmds;

    // Web server resources
    RdWebServerResourceDescr* _pWebServerResources;
    int _numWebServerResources;

    // HTTP response header - see comment above about single threaded web server
    char _httpRespHeader[HTTPD_MAX_HDR_LENGTH+1];
    void formHTTPResponse(char* pRespHeader, const char* rsltCode, const char* contentType, const char* respStr, int contentLen);

    // Utility
    static void formStringFromCharBuf(String& outStr, char* pStr, int len);
    void setState(WebServerState newState);

  public:
    RdWebServer(WiFiConn* pWiFiConn);
    virtual ~RdWebServer();

    void start(int port);
    void stop();
    void restart();
    void service();
    const char* connStateStr();
    char connStateChar();
    int connState() { return _webServerState; };

    // Add a command to the web server
    void addCommand(char* pCmdStr, int cmdType, CmdCallbackType callback = NULL);

    // Add resources to the web server
    void addStaticResources(RdWebServerResourceDescr* pResources, int numResources);

    // Get number of web commands
    int getNumWebCommands();

    // Get nth web command string
    char* getNthWebCmd(int n, int& cmdType, CmdCallbackType& callback);

    // Methods
    static const int METHOD_OTHER = 0;
    static const int METHOD_GET = 1;
    static const int METHOD_POST = 2;
    static const int METHOD_OPTIONS = 3;

    // Helpers
    static unsigned char* getPayloadDataFromMsg(char* msgBuf, int msgLen, int& payloadLen);
    static int getContentLengthFromMsg(char* msgBuf);
    static int getNumArgs(char* argStr);
    static char* getArgPtrAndLen(char* argStr, int argIdx, int& argLen);
    static String unencodeHTTPChars(String& inStr);
    static String getNthArgStr(char* argStr, int argIdx);

};

#endif
