// Web Server (target Particle devices e.g. RedBear Duo)
// Rob Dobson 2012-2017

#include "application.h"
#include "RdWebServer.h"
#include "Utils.h"

RdWebClient::RdWebClient()
{
    _webClientState = WEB_CLIENT_NONE;
    _webClientStateEntryMs = 0;
}


void RdWebClient::setState(WebClientState newState)
{
    _webClientState        = newState;
    _webClientStateEntryMs = millis();
    Log.trace("WebClientState: %s", connStateStr());
}

const char *RdWebClient::connStateStr()
{
    switch (_webClientState)
    {
    case WEB_CLIENT_NONE:
        return "None";

    case WEB_CLIENT_ACCEPTED:
        return "Accepted";
    }
    return "Unknown";
}

//////////////////////////////////
// Handle the client state machine
void RdWebClient::service(RdWebServer* pWebServer)
{
    // Handle different states
    switch (_webClientState)
    {
    case WEB_CLIENT_NONE:
        // See if a connection is ready to be accepted
        _TCPClient = pWebServer->available();
        if (_TCPClient)
        {
            // Now connected
            setState(WEB_CLIENT_ACCEPTED);
            _httpReqStr = "";
            // Info
            IPAddress ip    = _TCPClient.remoteIP();
            String    ipStr = ip;
            Log.info("Web client IP %s", ipStr.c_str());
        }
        break;

    case WEB_CLIENT_ACCEPTED:
       {
           // Check if client is still connected
           if (!_TCPClient.connected())
           {
               Log.trace("Web client disconnected");
               _TCPClient.stop();
               setState(WEB_CLIENT_NONE);
               break;
           }

           // Anything available?
           int numBytesAvailable = _TCPClient.available();
           int numToRead = numBytesAvailable;
           if (numToRead > 0)
           {
               // Make sure we don't overflow buffers
               if (numToRead > MAX_CHS_IN_SERVICE_LOOP)
               {
                   numToRead = MAX_CHS_IN_SERVICE_LOOP;
               }
               if (_httpReqStr.length() + numToRead > HTTPD_MAX_REQ_LENGTH)
               {
                   numToRead = HTTPD_MAX_REQ_LENGTH - _httpReqStr.length();
               }
           }

           // Check if we want to read
           if (numToRead > 0)
           {
               // Read from TCP client
               uint8_t *tmpBuf = new uint8_t[numToRead + 1];
               int  numRead = _TCPClient.read(tmpBuf, numToRead);
               tmpBuf[numToRead] = '\0';
               _httpReqStr.concat((char*)tmpBuf);
               delete [] tmpBuf;

               // Check for buffer full or blank line received
               if ((_httpReqStr.length() >= HTTPD_MAX_REQ_LENGTH) ||
                   (_httpReqStr.indexOf("\r\n\r\n") >= 0))
               {
                   pWebServer->handleReceivedHttp(_httpReqStr.c_str(), _httpReqStr.length(), _TCPClient);
                   // Close the connection now that we have responded
                   Log.trace("Web client response complete");
                   _TCPClient.stop();
                   setState(WEB_CLIENT_NONE);
               }
               else
               {
                   // Restart state timer to ensure timeout only happens when there is no data
                   Log.info("Web client rx %d chars", numRead);
                   _webClientStateEntryMs = millis();
               }
           }
           // Check for having been in this state for too long
           if (Utils::isTimeout(millis(), _webClientStateEntryMs, MAX_MS_IN_CLIENT_STATE_WITHOUT_DATA))
           {
               Log.info("Web client no-data timeout");
               _TCPClient.stop();
               setState(WEB_CLIENT_NONE);
           }
           break;
       }
    }
}

RdWebServer::RdWebServer()
{
    _pTCPServer              = NULL;
    _TCPPort                 = 80;
    _webServerState          = WEB_SERVER_STOPPED;
    _webServerStateEntryMs   = 0;
    _numWebServerResources   = 0;
}


// Destructor
RdWebServer::~RdWebServer()
{
}


const char *RdWebServer::connStateStr()
{
    switch (_webServerState)
    {
    case WEB_SERVER_STOPPED:
        return "Stopped";

    case WEB_SERVER_WAIT_CONN:
        return "WaitConn";

    case WEB_SERVER_BEGUN:
        return "Begun";
    }
    return "Unknown";
}


char RdWebServer::connStateChar()
{
    switch (_webServerState)
    {
    case WEB_SERVER_STOPPED:
        return '0';

    case WEB_SERVER_WAIT_CONN:
        return 'W';

    case WEB_SERVER_BEGUN:
        return 'B';
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
    setState(WEB_SERVER_WAIT_CONN);
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
    setState(WEB_SERVER_STOPPED);
}

TCPClient RdWebServer::available()
{
    if (_pTCPServer)
    {
        return _pTCPServer->available();
    }
    return NULL;
}


//////////////////////////////////////
// Handle the connection state machine
void RdWebServer::service()
{
    // Handle different states
    switch (_webServerState)
    {
    case WEB_SERVER_STOPPED:
        break;

    case WEB_SERVER_WAIT_CONN:
        if (WiFi.ready())
        {
            start(_TCPPort);
            if (_pTCPServer)
            {
                Log.info("Web Server TCPServer Begin");
                _pTCPServer->begin();
                setState(WEB_SERVER_BEGUN);
            }
        }
        break;

    case WEB_SERVER_BEGUN:
        // Service the clients
        for (int clientIdx = 0; clientIdx < MAX_WEB_CLIENTS; clientIdx++)
        {
            _webClients[clientIdx].service(this);
        }
        break;
    }
}


//////////////////////////////////////
// Handle an HTTP request
bool RdWebServer::handleReceivedHttp(const char *httpReq, int httpReqLen, TCPClient& tcpClient)
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
    String endpointStr, argStr;
    if (extractEndpointArgs(httpReq + 3, endpointStr, argStr, contentLen))
    {
        // Received cmd and arguments
        Log.trace("EndPtStr %s", endpointStr.c_str());
        Log.trace("ArgStr %s", argStr.c_str());

        // Handle REST API commands
        if (_pRestAPIEndpoints)
        {
            RestAPIEndpointDef *pEndpoint = _pRestAPIEndpoints->getEndpoint(endpointStr);
            if (pEndpoint)
            {
                Log.trace("FoundEndpoint <%s> Type %d", endpointStr.c_str(), pEndpoint->_endpointType);
                if (pEndpoint->_endpointType == RestAPIEndpointDef::ENDPOINT_CALLBACK)
                {
                    char *respStr = (pEndpoint->_callback)(httpMethod, endpointStr.c_str(), argStr.c_str(),
                                    httpReq, httpReqLen, contentLen, pPayload, payloadLen, 0);
                    String httpResp;
                    formHTTPResponse(httpResp, "200 OK", "application/json", respStr, -1);
                    tcpClient.write((uint8_t*)httpResp.c_str(), httpResp.length());
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
                if ((strcasecmp(pRes->_pResId, endpointStr) == 0) ||
                            ((strlen(endpointStr) == 0) && (strcasecmp(pRes->_pResId, "index.html") == 0)))
                {
                    if (pRes->_pData != NULL)
                    {
                        Log.trace("Sending resource %s, %d bytes, %s",
                                  pRes->_pResId, pRes->_dataLen, pRes->_pMimeType);
                        // Data to be sent
                        const unsigned char *testBuf = pRes->_pData;
                        int                 dataLen  = pRes->_dataLen;
                        // Form header
                        String httpResp;
                        formHTTPResponse(httpResp, "200 OK", pRes->_pMimeType, "", dataLen);
                        tcpClient.write((uint8_t*)httpResp.c_str(), httpResp.length());
                        tcpClient.flush();
                        delay(1);
                        // Send data in chunks based on limited buffer sizes in TCP stack
                        const unsigned char *pMem    = (const unsigned char *)testBuf;
                        int                 nLenLeft = dataLen;
                        int blkSize  = HTTPD_MAX_RESP_CHUNK_SIZE;
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
                            tcpClient.flush();
                        }
                        Log.info("Sent %s, %d bytes total, %d blocks",
                                 pRes->_pResId, pRes->_dataLen, blksSent);
                        handledOk = true;
                    }
                    break;
                }
            }
        }

        // If not handled ok
        if (!handledOk)
        {
            Log.info("Endpoint %s not found or invalid", endpointStr.c_str());
        }
    }
    else
    {
        Log.info("Cannot find command or args");
    }

    // Handle situations where the command wasn't handled ok
    if (!handledOk)
    {
        Log.info("Returning 404 Not found");
        String httpResp;
        formHTTPResponse(httpResp, "404 Not Found", "text/plain", "404 Not Found", -1);
        tcpClient.write((uint8_t*)httpResp.c_str(), httpResp.length());
    }
    return handledOk;
}


//////////////////////////////////////
// Extract arguments from rest api string
bool RdWebServer::extractEndpointArgs(const char *buf, String& endpointStr, String& argStr, int& contentLen)
{
    contentLen    = -1;
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
        if ((*pSlash1 == '/') || (*pSlash1 == ' ') || (*pSlash1 == '\n') ||
            (*pSlash1 == '?') || (*pSlash1 == '&'))
        {
            break;
        }
        endpointStr.concat(*pSlash1++);
    }
    if ((*pSlash1 == '\0') || (*pSlash1 == ' ') || (*pSlash1 == '\n'))
    {
        return true;
    }
    // Now args
    pSlash1++;
    while (*pSlash1)
    {
        if ((*pSlash1 == ' ') || (*pSlash1 == '\n'))
        {
            break;
        }
        argStr.concat(*pSlash1++);
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


unsigned char *RdWebServer::getPayloadDataFromMsg(const char *msgBuf, int msgLen, int& payloadLen)
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


int RdWebServer::getContentLengthFromMsg(const char *msgBuf)
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
void RdWebServer::formHTTPResponse(String& respStr, const char *rsltCode,
                    const char *contentType, const char *respBody, int contentLen)
{
    if (contentLen == -1)
    {
        contentLen = strlen(respBody);
    }
    respStr = String::format("HTTP/1.1 %s\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: %s\r\nConnection: close\r\nContent-Length: %d\r\n\r\n%s", rsltCode, contentType, contentLen, respBody);
}
