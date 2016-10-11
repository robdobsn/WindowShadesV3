// Web Server (target Particle devices e.g. RedBear Duo)
// Rob Dobson 2012-2016

#include "application.h"
#include "WiFiConn.h"
#include "RdWebServer.h"

#define RD_DEBUG_LEVEL 4
#define RD_DEBUG_FNAME "RdWebServer.cpp"
#include "RdDebugLevel.h"

RdWebServer::RdWebServer(WiFiConn* pWiFiConn)
{
  _pTCPServer = NULL;
  _TCPPort = 80;
  _webServerState = WEB_SERVER_OFFLINE;
  _webServerStateEntryMs = 0;
  _pWiFiConn = pWiFiConn;
  _httpReqBufPos = 0;
  _charsOnHttpReqLine = 0;
  _numWebServerCmds = 0;
  _numWebServerResources = 0;
}

// Destructor
RdWebServer::~RdWebServer()
{
    // Clean-up - probably never called as we're on a microcontroller!
    for (int i = 0; i < _numWebServerCmds; i++)
        delete _pWebServerCmds[i];
    _numWebServerCmds = 0;
}

const char* RdWebServer::connStateStr()
{
  switch(_webServerState)
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
    switch(_webServerState)
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
    _webServerState = newState;
    _webServerStateEntryMs = millis();
    RD_DBG("WebServerState: %s", connStateStr());
}

void RdWebServer::start(int port)
{
  // Check if already started
  if (_pTCPServer)
  {
        // // Check if the port hasn't changed - if so nothing to do
        // if (_TCPPort == port)
        //     return;
        stop();
  }
  // Create server and begin
  _pTCPServer = new TCPServer(port);
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
    return;
  // Handle different states
  switch(_webServerState)
  {
    case WEB_SERVER_WAIT_CONNECT:
    {
      if (_pWiFiConn->isConnected())
      {
        _pTCPServer->begin();
                Serial.println("****************** TCP Server Begin");
        setState(WEB_SERVER_CONNECTED_BUT_NO_CLIENT);
      }
      break;
    }
    case WEB_SERVER_CONNECTED_BUT_NO_CLIENT:
    {
      // Check for WiFi disconnect
      if (!_pWiFiConn->isConnected())
      {
        RD_DBG("Web Server was connected but WiFi lost ...");
                restart();
        break;
      }
      // Check for a client
      _TCPClient = _pTCPServer->available();
      if (_TCPClient)
      {
        setState(WEB_SERVER_HAS_CLIENT);
        _httpReqBufPos = 0;
        _charsOnHttpReqLine = 0;
#if (RD_DEBUG_LEVEL > 3)
        IPAddress ip = _TCPClient.remoteIP();
        String ipStr = ip;
        char ipStrBuf[20];
        ipStr.toCharArray(ipStrBuf, 19);
        RD_DBG("Web Server Client IP %s", ipStrBuf);
#endif
      }
      break;
    }
    case WEB_SERVER_HAS_CLIENT:
    {
      // Check for WiFi disconnect
      if (!_pWiFiConn->isConnected())
      {
        _TCPClient.stop();
                restart();
        RD_DBG("Web Server had client but WiFi lost -> Wait Connect");
        break;
      }
      // Check if client is still connected
      if (!_TCPClient.connected())
      {
        _TCPClient.stop();
        setState(WEB_SERVER_CONNECTED_BUT_NO_CLIENT);
        RD_DBG("Web Server had client but client disconnected -> Connected");
        break;
      }
      // Check for data - but only a few chars per loop to give other
      // parts of the code a chance to run
      bool anyDataReceived = false;
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
            RD_DBG("Web Server response complete - Client connection stopped");
            _TCPClient.stop();
            setState(WEB_SERVER_CONNECTED_BUT_NO_CLIENT);
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
      // Re-enter state to ensure timeout only happens when there is no data
      if (anyDataReceived)
      {
          setState(WEB_SERVER_HAS_CLIENT);
      }
      // Check for having been in this state for too long
      if (Utils::isTimeout(millis(), _webServerStateEntryMs, MAX_MS_IN_CLIENT_STATE_WITHOUT_DATA))
      {
        _TCPClient.stop();
        RD_DBG("Web Server had client but no-data timeout - Client connection stopped");
        setState(WEB_SERVER_CONNECTED_BUT_NO_CLIENT);
      }
      break;
    }
  }
}

//////////////////////////////////////
// Handle an HTTP request
bool RdWebServer::handleReceivedHttp(char* httpReq, int httpReqLen, TCPClient& tcpClient)
{
    bool handledOk = false;

    // Get payload information
    int payloadLen = -1;
    unsigned char* pPayload = getPayloadDataFromMsg(httpReq, httpReqLen, payloadLen);

    // Get HTTP method
    int httpMethod = METHOD_OTHER;
    if (strncmp(httpReq, "GET ", 4) == 0)
        httpMethod = METHOD_GET;
    else if (strncmp(httpReq, "POST", 4) == 0)
        httpMethod = METHOD_POST;
    else if (strncmp(httpReq, "OPTIONS", 7) == 0)
        httpMethod = METHOD_OPTIONS;

    // See if there is a valid HTTP command
    int contentLen = -1;
    char cmdStr[MAX_CMDSTR_LEN];
    char argStr[MAX_ARGSTR_LEN];
    if (extractCmdArgs(httpReq+3, cmdStr, MAX_CMDSTR_LEN, argStr, MAX_ARGSTR_LEN, contentLen))
    {
      // Received cmd and arguments
      RD_DBG("CmdStr %s", cmdStr);
      RD_DBG("ArgStr %s", argStr);

      // Look for the command in the registered callbacks
      for (int wsCmdIdx = 0; wsCmdIdx < _numWebServerCmds; wsCmdIdx++)
      {
        RdWebServerCmdDef* pCmd = _pWebServerCmds[wsCmdIdx];
        if (strcasecmp(pCmd->_pCmdStr, cmdStr) == 0)
        {
          RD_DBG("FoundCmd <%s> Type %d", cmdStr, pCmd->_cmdType);
          if (pCmd->_cmdType == RdWebServerCmdDef::CMD_CALLBACK)
          {
            char* respStr = (pCmd->_callback)(httpMethod, cmdStr, argStr, httpReq, httpReqLen,
                            contentLen, pPayload, payloadLen, 0);
            formHTTPResponse(_httpRespHeader, "200 OK", "application/json", respStr, -1);
            tcpClient.print(_httpRespHeader);
            handledOk = true;
          }
          break;
        }
      }

      // Look for the command in the static resources
      for (int wsResIdx = 0; wsResIdx < _numWebServerResources; wsResIdx++)
      {
        RdWebServerResourceDescr* pRes = &(_pWebServerResources[wsResIdx]);
        if ((strcasecmp(pRes->_pResId, cmdStr) == 0) || ((strlen(cmdStr) == 0) && (strcasecmp(pRes->_pResId, "index.html") == 0)))
        {
          if (pRes->_pData != NULL)
          {
            RD_INFO("Sending resource %s, %d bytes, %s", pRes->_pResId, pRes->_dataLen, pRes->_pMimeType);
            // Data to be sent
            const unsigned char* testBuf = pRes->_pData;
            int dataLen = pRes->_dataLen;
            // Form header
            formHTTPResponse(_httpRespHeader, "200 OK", pRes->_pMimeType, "", dataLen);
            tcpClient.print(_httpRespHeader);
            // Send data in chunks based on limited buffer sizes in TCP stack
            const unsigned char* pMem = (const unsigned char*) testBuf;
            int nLenLeft = dataLen;
            // RedBear Duo seems to be ok with this block size
            int blkSize = 500;
            while(nLenLeft > 0)
            {
                if (blkSize > nLenLeft)
                    blkSize = nLenLeft;
                RD_DBG("Sending %d", blkSize);
                tcpClient.write(pMem, blkSize);
                pMem += blkSize;
                nLenLeft -= blkSize;
                // Delay here seems necessary on RedBear Duo
                // Without this delay sometimes the TCP stack seems to get screwed up
                // And all subsequent attempts to send a page result 
                // in "failed - net::ERR_CONTENT_LENGTH_MISMATCH" in Chrome browser
                delay(1);
            }
            handledOk = true;
          }
          break;
        }
      }
      // If not handled ok
      if (!handledOk)
      {
        RD_DBG("Command %s not found or invalid", cmdStr);
      }
    }
    else
    {
        RD_DBG("Cannot find command or args");
    }

    // Handle situations where the command wasn't handled ok
    if (!handledOk)
    {
      RD_INFO("Returning 404 Not found");
      formHTTPResponse(_httpRespHeader, "404 Not Found", "text/plain", "404 Not Found", -1);
      tcpClient.print(_httpRespHeader);
    }
    return handledOk;
}

//////////////////////////////////////
// Extract arguments from command
bool RdWebServer::extractCmdArgs(char* buf, char* pCmdStr, int maxCmdStrLen, char* pArgStr, int maxArgStrLen, int& contentLen)
{
    contentLen = -1;
    *pCmdStr = '\0';
    *pArgStr = '\0';
    int cmdStrLen = 0;
    int argStrLen = 0;
    if (buf == NULL)
        return false;
    // Check for Content-length header
    const char* contentLenText = "Content-Length:";
    char* pContLen = strstr(buf, contentLenText);
    if (pContLen)
    {
        if (*(pContLen + strlen(contentLenText)) != '\0')
            contentLen = atoi(pContLen + strlen(contentLenText));
    }

    // Check for first slash
    char* pSlash1 = strchr(buf, '/');
    if (pSlash1 == NULL)
        return false;
    pSlash1++;
    // Extract command
    while(*pSlash1)
    {
        if (cmdStrLen >= maxCmdStrLen-1)
            break;
        if ((*pSlash1 == '/') || (*pSlash1 == ' ') || (*pSlash1 == '\n') || (*pSlash1 == '?') || (*pSlash1 == '&'))
            break;
        *pCmdStr++ = *pSlash1++;
        *pCmdStr = '\0';
        cmdStrLen++;
    }
    if ((*pSlash1 == '\0') || (*pSlash1 == ' ') || (*pSlash1 == '\n'))
        return true;
    // Now args
    pSlash1++;
    while(*pSlash1)
    {
        if (argStrLen >= maxArgStrLen-1)
            break;
        if ((*pSlash1 == ' ') || (*pSlash1 == '\n'))
            break;
        *pArgStr++ = *pSlash1++;
        *pArgStr = '\0';
        argStrLen++;
    }
    return true;
}

//////////////////////////////////////
// Add a command to the server
void RdWebServer::addCommand(char* pCmdStr, int cmdType, CmdCallbackType callback)
{
    // Check for overflow
    if (_numWebServerCmds >= MAX_WEB_SERVER_CMDS)
        return;

    // Create new command definition and add
    RdWebServerCmdDef* pNewCmdDef = new RdWebServerCmdDef(pCmdStr, cmdType, callback);
    _pWebServerCmds[_numWebServerCmds] = pNewCmdDef;
    _numWebServerCmds++;
}

// Get number of web commands
int RdWebServer::getNumWebCommands()
{
    return _numWebServerCmds;
}

// Get nth web command string
char* RdWebServer::getNthWebCmd(int n, int& cmdType, CmdCallbackType& callback)
{
    if (n >= 0 && n < _numWebServerCmds)
    {
        cmdType = _pWebServerCmds[n]->_cmdType;
        callback = _pWebServerCmds[n]->_callback;
        return _pWebServerCmds[n]->_pCmdStr;
    }
    return NULL;
}

// Add resources to the web server
void RdWebServer::addStaticResources(RdWebServerResourceDescr* pResources, int numResources)
{
  _pWebServerResources = pResources;
  _numWebServerResources = numResources;
}

unsigned char* RdWebServer::getPayloadDataFromMsg(char* msgBuf, int msgLen, int& payloadLen)
{
    payloadLen = -1;
    char* ptr = strstr(msgBuf, "\r\n\r\n");
    if (ptr)
    {
        payloadLen = msgLen - (ptr+4-msgBuf);
        return (unsigned char*) (ptr+4);
    }
    return NULL;
}

int RdWebServer::getContentLengthFromMsg(char* msgBuf)
{
    char* ptr = strstr(msgBuf, "Content-Length:");
    if (ptr)
    {
        ptr += 15;
        int contentLen = atoi(ptr);
        if (contentLen >= 0)
            return contentLen;
    }
    return 0;
}

// Form a header to respond
void RdWebServer::formHTTPResponse(char* pRespHeader, const char* rsltCode, const char* contentType, const char* respStr, int contentLen)
{
  if (contentLen == -1)
    contentLen = strlen(respStr);
  sprintf(pRespHeader, "HTTP/1.1 %s\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: %s\r\nConnection: close\r\nContent-Length: %d\r\n\r\n%s", rsltCode, contentType, contentLen, respStr);
}

// Num args from an argStr
int RdWebServer::getNumArgs(char* argStr)
{
  int numArgs = 0;
  int numChSinceSep = 0;
  char* pCh = argStr;
  // Count args
  while(*pCh)
  {
    if (*pCh == '/')
    {
      numArgs++;
      numChSinceSep = 0;
    }
    pCh++;
    numChSinceSep++;
  }
  if (numChSinceSep > 0)
    return numArgs+1;
  return numArgs;
}

// Get position and length of nth arg
char* RdWebServer::getArgPtrAndLen(char* argStr, int argIdx, int& argLen)
{
  int curArgIdx = 0;
  char* pCh = argStr;
  char* pArg = argStr;
  while(true)
  {
    if ((*pCh == '/') || (*pCh == '\0'))
    {
      if (curArgIdx == argIdx)
      {
        argLen = pCh - pArg;
        return pArg;
      }
      if (*pCh == '\0')
        return NULL;
      pArg = pCh+1;
      curArgIdx++;
    }
    pCh++;
  }
  return NULL;
}

// Convert encoded URL
String RdWebServer::unencodeHTTPChars(String& inStr)
{
  inStr.replace("+", " ");
  inStr.replace("%20", " ");
  inStr.replace("%21", "!");
  inStr.replace("%22", "\"");
  inStr.replace("%23", "#");
  inStr.replace("%24", "$");
  inStr.replace("%25", "%");
  inStr.replace("%26", "&");
  inStr.replace("%27", "^");
  inStr.replace("%28", "(");
  inStr.replace("%29", ")");
  inStr.replace("%2A", "*");
  inStr.replace("%2B", "+");
  inStr.replace("%2C", ",");
  inStr.replace("%2D", "-");
  inStr.replace("%2E", ".");
  inStr.replace("%2F", "/");
  inStr.replace("%3A", ":");
  inStr.replace("%3B", ";");
  inStr.replace("%3C", "<");
  inStr.replace("%3D", "=");
  inStr.replace("%3E", ">");
  inStr.replace("%3F", "?");
  inStr.replace("%5B", "[");
  inStr.replace("%5C", "\\");
  inStr.replace("%5D", "]");
  inStr.replace("%5E", "^");
  inStr.replace("%5F", "_");
  inStr.replace("%60", "`");
  inStr.replace("%7B", "{");
  inStr.replace("%7C", "|");
  inStr.replace("%7D", "}");
  inStr.replace("%7E", "~");
  return inStr;
}

void RdWebServer::formStringFromCharBuf(String& outStr, char* pStr, int len)
{
  outStr = "";
  for (int i = 0; i < len; i++)
  {
    outStr.concat(*pStr);
    pStr++;
  }
}

String RdWebServer::getNthArgStr(char* argStr, int argIdx)
{
  int argLen = 0;
  String oStr;
  char* pStr = RdWebServer::getArgPtrAndLen(argStr, argIdx, argLen);
  if (pStr)
    formStringFromCharBuf(oStr, pStr, argLen);
  oStr = unencodeHTTPChars(oStr);
  return oStr;
}
