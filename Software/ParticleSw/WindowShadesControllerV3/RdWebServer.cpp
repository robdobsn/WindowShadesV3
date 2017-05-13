// Web Server (target Particle devices e.g. RedBear Duo)
// Rob Dobson 2012-2017

#include "application.h"
#include "RdWebServer.h"
#include "Utils.h"

RdWebServer::RdWebServer()
{
    _pTCPServer              = NULL;
    _TCPPort                 = 80;
    _webServerState          = WEB_SERVER_OFFLINE;
    _webServerStateEntryMs   = 0;
    _httpReqBufPos           = 0;
    _charsOnHttpReqLine      = 0;
    _numWebServerResources   = 0;
    _lastWebServerResponseMs = 0;
}


// Destructor
RdWebServer::~RdWebServer()
{
}


const char *RdWebServer::connStateStr()
{
    switch (_webServerState)
    {
    case WEB_SERVER_OFFLINE:
        return "Offline";

    case WEB_SERVER_WAIT_CONNECT:
        return "WaitConn";

    case WEB_SERVER_CONNECTED_BUT_NO_CLIENT:
        return "ConnectedButNoClient";

    case WEB_SERVER_HAS_CLIENT:
        return "HasClient";
    }
    return "Unknown";
}


char RdWebServer::connStateChar()
{
    switch (_webServerState)
    {
    case WEB_SERVER_OFFLINE:
        return '0';

    case WEB_SERVER_WAIT_CONNECT:
        return 'W';

    case WEB_SERVER_CONNECTED_BUT_NO_CLIENT:
        return 'C';

    case WEB_SERVER_HAS_CLIENT:
        return 'H';
    }
    return 'K';
}


void RdWebServer::setState(WebServerState newState)
{
    _webServerState        = newState;
    _webServerStateEntryMs = millis();
    Log.trace("WebServerState: %s", connStateStr());
}


void RdWebServer::start(int port)
{
    Log.trace("WebServerStart");
    // Check if already started
    if (_pTCPServer)
    {
        stop();
    }
    // Create server and begin
    _TCPPort    = port;
    _pTCPServer = new TCPServer(_TCPPort);
    setState(WEB_SERVER_WAIT_CONNECT);
}


void RdWebServer::stop()
{
    if (_pTCPServer)
    {
        _pTCPServer->stop();
        // Delete previous server
        delete _pTCPServer;
        _pTCPServer = NULL;
    }
    setState(WEB_SERVER_OFFLINE);
}


void RdWebServer::restart()
{
    start(_TCPPort);
}


//////////////////////////////////////
// Handle the connection state machine
void RdWebServer::service()
{
    // Check initialised
    if (!_pTCPServer)
    {
        return;
    }

    // Handle different states
    switch (_webServerState)
    {
    case WEB_SERVER_OFFLINE:
        if (WiFi.ready())
        {
            restart();
            setState(WEB_SERVER_WAIT_CONNECT);
        }
        break;

    case WEB_SERVER_WAIT_CONNECT:
        if (WiFi.ready())
        {
            Log.trace("****************** TCP Server Begin");
            _pTCPServer->begin();
            setState(WEB_SERVER_CONNECTED_BUT_NO_CLIENT);
        }
        break;

    case WEB_SERVER_CONNECTED_BUT_NO_CLIENT:
        // Check for WiFi disconnect
        if (!WiFi.ready())
        {
            Log.trace("Web Server was connected but WiFi lost ...");
            restart();
            break;
        }
        // Check for a client
        _TCPClient = _pTCPServer->available();
        if (_TCPClient)
        {
            setState(WEB_SERVER_HAS_CLIENT);
            _httpReqBufPos      = 0;
            _charsOnHttpReqLine = 0;
            if (Log.isTraceEnabled())
            {
                IPAddress ip    = _TCPClient.remoteIP();
                String    ipStr = ip;
                char      ipStrBuf[20];
                ipStr.toCharArray(ipStrBuf, 19);
                Log.trace("Web Server Client IP %s", ipStrBuf);
            }
        }
        break;

    case WEB_SERVER_HAS_CLIENT:
       {
           // Check for WiFi disconnect
           if (!WiFi.ready())
           {
               Log.trace("Web Server had client but WiFi lost -> Wait Connect");
               _TCPClient.stop();
               restart();
               break;
           }
           // Check if client is still connected
           if (!_TCPClient.connected())
           {
               Log.trace("Web Server had client but client disconnected -> Connected");
               _TCPClient.stop();
               setState(WEB_SERVER_CONNECTED_BUT_NO_CLIENT);
               break;
           }
           // Check for data - but only a few chars per loop to give other
           // parts of the code a chance to run
           int  debugCurHttpBufPos = _httpReqBufPos;
           bool anyDataReceived    = false;
           for (int chIdx = 0; chIdx < MAX_CHS_IN_SERVICE_LOOP; chIdx++)
           {
               if (_TCPClient.available())
               {
                   // Get the char and put in buffer
                   char ch = _TCPClient.read();
                   _httpReqBuf[_httpReqBufPos++] = ch;
                   // Check for a blank line received or buffer full
                   if (((ch == '\n') && (_charsOnHttpReqLine == 0)) ||
                       (_httpReqBufPos >= HTTPD_MAX_REQ_LENGTH))
                   {
                       // Terminate the buffer and process
                       _httpReqBuf[_httpReqBufPos] = '\0';
                       handleReceivedHttp(_httpReqBuf, _httpReqBufPos, _TCPClient);
                       // Close the connection now that we have responded
                       Log.trace("Web Server response complete - Client connection stopped");
                       _TCPClient.stop();
                       setState(WEB_SERVER_CONNECTED_BUT_NO_CLIENT);
                       _lastWebServerResponseMs = millis();
                       break;
                   }
                   // Check what char was received last
                   if (ch == '\n')
                   {
                       _charsOnHttpReqLine = 0;
                   }
                   else if (ch != '\r')
                   {
                       _charsOnHttpReqLine++;
                   }
                   anyDataReceived = true;
               }
           }
           // Restart state timer to ensure timeout only happens when there is no data
           if (anyDataReceived)
           {
               Log.trace("Received %d chars", _httpReqBufPos - debugCurHttpBufPos);
               _webServerStateEntryMs = millis();
           }
           // Check for having been in this state for too long
           if (Utils::isTimeout(millis(), _webServerStateEntryMs, MAX_MS_IN_CLIENT_STATE_WITHOUT_DATA))
           {
               Log.trace("Web Server had client but no-data timeout - Client connection stopped");
               _TCPClient.stop();
               setState(WEB_SERVER_CONNECTED_BUT_NO_CLIENT);
           }
           break;
       }
    }
}


//////////////////////////////////////
// Handle an HTTP request
bool RdWebServer::handleReceivedHttp(char *httpReq, int httpReqLen, TCPClient& tcpClient)
{
    bool handledOk = false;

    // Get payload information
    int           payloadLen = -1;
    unsigned char *pPayload  = getPayloadDataFromMsg(httpReq, httpReqLen, payloadLen);

    // Get HTTP method
    int httpMethod = METHOD_OTHER;

    if (strncmp(httpReq, "GET ", 4) == 0)
    {
        httpMethod = METHOD_GET;
    }
    else if (strncmp(httpReq, "POST", 4) == 0)
    {
        httpMethod = METHOD_POST;
    }
    else if (strncmp(httpReq, "OPTIONS", 7) == 0)
    {
        httpMethod = METHOD_OPTIONS;
    }

    // See if there is a valid HTTP command
    int  contentLen = -1;
    char endpointStr[MAX_ENDPOINT_LEN];
    char argStr[MAX_ARGSTR_LEN];
    if (extractEndpointArgs(httpReq + 3, endpointStr, MAX_ENDPOINT_LEN, argStr, MAX_ARGSTR_LEN, contentLen))
    {
        // Received cmd and arguments
        Log.trace("EndPtStr %s", endpointStr);
        Log.trace("ArgStr %s", argStr);

        // Handle REST API commands
        if (_pRestAPIEndpoints)
        {
            RestAPIEndpointDef *pEndpoint = _pRestAPIEndpoints->getEndpoint(endpointStr);
            if (pEndpoint)
            {
                Log.trace("FoundEndpoint <%s> Type %d", endpointStr, pEndpoint->_endpointType);
                if (pEndpoint->_endpointType == RestAPIEndpointDef::ENDPOINT_CALLBACK)
                {
                    char *respStr = (pEndpoint->_callback)(httpMethod, endpointStr, argStr, httpReq, httpReqLen,
                                                           contentLen, pPayload, payloadLen, 0);
                    formHTTPResponse(_httpRespHeader, "200 OK", "application/json", respStr, -1);
                    tcpClient.print(_httpRespHeader);
                    handledOk = true;
                }
            }
        }

        // Look for the command in the static resources
        if (!handledOk)
        {
            for (int wsResIdx = 0; wsResIdx < _numWebServerResources; wsResIdx++)
            {
                RdWebServerResourceDescr *pRes = &(_pWebServerResources[wsResIdx]);
                if ((strcasecmp(pRes->_pResId, endpointStr) == 0) || ((strlen(endpointStr) == 0) && (strcasecmp(pRes->_pResId, "index.html") == 0)))
                {
                    if (pRes->_pData != NULL)
                    {
                        Log.trace("Sending resource %s, %d bytes, %s",
                                  pRes->_pResId, pRes->_dataLen, pRes->_pMimeType);
                        // Data to be sent
                        const unsigned char *testBuf = pRes->_pData;
                        int                 dataLen  = pRes->_dataLen;
                        // Form header
                        formHTTPResponse(_httpRespHeader, "200 OK", pRes->_pMimeType, "", dataLen);
                        tcpClient.print(_httpRespHeader);
                        // Send data in chunks based on limited buffer sizes in TCP stack
                        const unsigned char *pMem    = (const unsigned char *)testBuf;
                        int                 nLenLeft = dataLen;
                        // RedBear Duo seems to be ok with this block size
                        int blkSize  = 500;
                        int blksSent = 0;
                        while (nLenLeft > 0)
                        {
                            if (blkSize > nLenLeft)
                            {
                                blkSize = nLenLeft;
                            }
                            tcpClient.write(pMem, blkSize);
                            pMem     += blkSize;
                            nLenLeft -= blkSize;
                            blksSent++;
                            // Delay here seems necessary on RedBear Duo
                            // Without this delay sometimes the TCP stack seems to get screwed up
                            // And all subsequent attempts to send a page result
                            // in "failed - net::ERR_CONTENT_LENGTH_MISMATCH" in Chrome browser
                            delay(1);
                        }
                        Log.trace("Sent %s, %d bytes total, %d blocks of %d bytes",
                                  pRes->_pResId, pRes->_dataLen, blksSent, blkSize);
                        handledOk = true;
                    }
                    break;
                }
            }
        }

        // If not handled ok
        if (!handledOk)
        {
            Log.trace("Endpoint %s not found or invalid", endpointStr);
        }
    }
    else
    {
        Log.trace("Cannot find command or args");
    }

    // Handle situations where the command wasn't handled ok
    if (!handledOk)
    {
        Log.info("Returning 404 Not found");
        formHTTPResponse(_httpRespHeader, "404 Not Found", "text/plain", "404 Not Found", -1);
        tcpClient.print(_httpRespHeader);
    }
    return handledOk;
}


//////////////////////////////////////
// Extract arguments from rest api string
bool RdWebServer::extractEndpointArgs(char *buf, char *pEndpointStr, int maxEndpointStrLen,
                                      char *pArgStr, int maxArgStrLen, int& contentLen)
{
    contentLen    = -1;
    *pEndpointStr = '\0';
    *pArgStr      = '\0';
    int endpointStrLen = 0;
    int argStrLen      = 0;
    if (buf == NULL)
    {
        return false;
    }
    // Check for Content-length header
    const char *contentLenText = "Content-Length:";
    char       *pContLen       = strstr(buf, contentLenText);
    if (pContLen)
    {
        if (*(pContLen + strlen(contentLenText)) != '\0')
        {
            contentLen = atoi(pContLen + strlen(contentLenText));
        }
    }

    // Check for first slash
    char *pSlash1 = strchr(buf, '/');
    if (pSlash1 == NULL)
    {
        return false;
    }
    pSlash1++;
    // Extract command
    while (*pSlash1)
    {
        if (endpointStrLen >= maxEndpointStrLen - 1)
        {
            break;
        }
        if ((*pSlash1 == '/') || (*pSlash1 == ' ') || (*pSlash1 == '\n') ||
            (*pSlash1 == '?') || (*pSlash1 == '&'))
        {
            break;
        }
        *pEndpointStr++ = *pSlash1++;
        *pEndpointStr   = '\0';
        endpointStrLen++;
    }
    if ((*pSlash1 == '\0') || (*pSlash1 == ' ') || (*pSlash1 == '\n'))
    {
        return true;
    }
    // Now args
    pSlash1++;
    while (*pSlash1)
    {
        if (argStrLen >= maxArgStrLen - 1)
        {
            break;
        }
        if ((*pSlash1 == ' ') || (*pSlash1 == '\n'))
        {
            break;
        }
        *pArgStr++ = *pSlash1++;
        *pArgStr   = '\0';
        argStrLen++;
    }
    return true;
}


//////////////////////////////////////
// Add resources to the web server
void RdWebServer::addStaticResources(RdWebServerResourceDescr *pResources, int numResources)
{
    _pWebServerResources   = pResources;
    _numWebServerResources = numResources;
}


// Add endpoints to the web server
void RdWebServer::addRestAPIEndpoints(RestAPIEndpoints *pRestAPIEndpoints)
{
    _pRestAPIEndpoints = pRestAPIEndpoints;
}


unsigned char *RdWebServer::getPayloadDataFromMsg(char *msgBuf, int msgLen, int& payloadLen)
{
    payloadLen = -1;
    char *ptr = strstr(msgBuf, "\r\n\r\n");
    if (ptr)
    {
        payloadLen = msgLen - (ptr + 4 - msgBuf);
        return (unsigned char *)(ptr + 4);
    }
    return NULL;
}


int RdWebServer::getContentLengthFromMsg(char *msgBuf)
{
    char *ptr = strstr(msgBuf, "Content-Length:");

    if (ptr)
    {
        ptr += 15;
        int contentLen = atoi(ptr);
        if (contentLen >= 0)
        {
            return contentLen;
        }
    }
    return 0;
}


// Form a header to respond
void RdWebServer::formHTTPResponse(char *pRespHeader, const char *rsltCode,
                                   const char *contentType, const char *respStr, int contentLen)
{
    if (contentLen == -1)
    {
        contentLen = strlen(respStr);
    }
    sprintf(pRespHeader, "HTTP/1.1 %s\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: %s\r\nConnection: close\r\nContent-Length: %d\r\n\r\n%s", rsltCode, contentType, contentLen, respStr);
}
