// RinggitTheSalesBell
// Rob Dobson 2018

#pragma once

typedef void (*SerialConsoleCallbackType)(const char* cmdStr, String& retStr);

class SerialConsole
{
private:
    int _serialPortNum;
    String _curLine;
    static const int MAX_REGULAR_LINE_LEN = 100;
    static const int ABS_MAX_LINE_LEN = 1000;
    SerialConsoleCallbackType _apiCallback;

public:
    SerialConsole(int serialPortNum, SerialConsoleCallbackType apiCallback)
    {
        _serialPortNum = serialPortNum;
        _curLine.reserve(MAX_REGULAR_LINE_LEN);
        _apiCallback = apiCallback;
    }

    int getChar()
    {
        if (_serialPortNum == 0)
        {
            // Get char
            return Serial.read();
        }
        return -1;
    }

    void service()
    {
        // Check for char
        int ch = getChar();
        if (ch == -1)
            return;

        // Check for line end
        if ((ch == '\r') || (ch == '\n'))
        {
            // Check if empty line - show menu
            if (_curLine.length() <= 0)
            {
                Serial.println("Configuration Options");
                Serial.println("Q - query status including IP address");
                Serial.println("W/ssid/password/hostname - setup wifi");
                Serial.println("WC - clear wifi ssid table");
                Serial.println("RESET - restart unit");
                return;
            }

            Serial.println();
            // Check for immediate instructions
            Log.trace(F("CommsSerial ->cmdInterp cmdStr %s"CR), _curLine.c_str());
            String retStr;
            _apiCallback(_curLine.c_str(), retStr);

            // Display response
            Serial.println(retStr);

            // Reset line
            _curLine = "";
            return;
        }

        // Check line not too long
        if (_curLine.length() >= ABS_MAX_LINE_LEN)
        {
            return;
        }

        // Check for backspace
        if (ch == 0x08)
        {
            if (_curLine.length() > 0)
            {
                _curLine.remove(_curLine.length()-1);
                Serial.print((char)ch);
                Serial.print(' ');
                Serial.print((char)ch);
            }
            return;
        }

        // Add char to line
        _curLine.concat((char)ch);
        Serial.print((char)ch);

        //Log.trace(F("Str = %s (%c)"CR), _curLine.c_str(), ch);
    }
};
