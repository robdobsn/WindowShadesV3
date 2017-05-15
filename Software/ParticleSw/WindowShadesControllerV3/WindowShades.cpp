// WindowShades
// Rob Dobson 2013-2017
// More details at http://robdobson.com/2013/10/moving-my-window-shades-control-to-mbed/

#include "WindowShades.h"
#include "Utils.h"

WindowShades::WindowShades(int hc595_SER, int hc595_SCK, int hc595_RCK)
{
    _hc595_SER = hc595_SER;
    pinMode(_hc595_SER, OUTPUT);
    _hc595_SCK = hc595_SCK;
    pinMode(_hc595_SCK, OUTPUT);
    _hc595_RCK = hc595_RCK;
    pinMode(_hc595_RCK, OUTPUT);
    for (int i = 0; i < MAX_WINDOW_SHADES; i++)
    {
        _msTimeouts[i] = 0;
        _tickCounts[i] = 0;
    }
    _curShadeCtrlBits = 0;
    setAllOutputs();
}


void WindowShades::setAllOutputs()
{
    Log.trace("Setting %08x", _curShadeCtrlBits);
    unsigned long dataVal = _curShadeCtrlBits;
    unsigned long bitMask = 1 << (MAX_WINDOW_SHADES * BITS_PER_SHADE - 1);
    // Send the value to the shift register
    for (int bitCount = 0; bitCount < MAX_WINDOW_SHADES * BITS_PER_SHADE; bitCount++)
    {
        // Set the data
        digitalWrite(_hc595_SER, (dataVal & bitMask) != 0);
        bitMask = bitMask >> 1;
        // Clock the data into the shift-register
        digitalWrite(_hc595_SCK, HIGH);
        delayMicroseconds(1);
        digitalWrite(_hc595_SCK, LOW);
        delayMicroseconds(1);
    }
    // Move the value into the output register
    digitalWrite(_hc595_RCK, HIGH);
    delayMicroseconds(1);
    digitalWrite(_hc595_RCK, LOW);
}


void WindowShades::setShadeBit(int shadeIdx, int bitMask, int bitIsOn)
{
    unsigned long movedMask = bitMask << (shadeIdx * BITS_PER_SHADE);

    if (bitIsOn)
    {
        _curShadeCtrlBits |= movedMask;
    }
    else
    {
        _curShadeCtrlBits &= (~movedMask);
    }
    setAllOutputs();
}


void WindowShades::clearShadeBits(int shadeIdx)
{
    // Clear any existing command
    _msTimeouts[shadeIdx] = 0;
    _tickCounts[shadeIdx] = 0;
    setShadeBit(shadeIdx, SHADE_UP_BIT_MASK | SHADE_STOP_BIT_MASK | SHADE_DOWN_BIT_MASK, false);
}


void WindowShades::setTimedOutput(int shadeIdx, int bitMask, bool bitOn, long msDuration, bool bClearExisting)
{
    Log.trace("TimedOutput\tidx %d\tmask %d\tbitOn %d\tduration %d\tclear %d", shadeIdx, bitMask, bitOn, msDuration, bClearExisting);

    if (bClearExisting)
    {
        clearShadeBits(shadeIdx);
    }
    setShadeBit(shadeIdx, bitMask, bitOn);
    if (msDuration != 0)
    {
        _msTimeouts[shadeIdx] = msDuration;
        _tickCounts[shadeIdx] = millis();
    }
}


void WindowShades::service()
{
    bool somethingSet = false;

    for (int shadeIdx = 0; shadeIdx < MAX_WINDOW_SHADES; shadeIdx++)
    {
        if (_msTimeouts[shadeIdx] != 0)
        {
            if (Utils::isTimeout(millis(), _tickCounts[shadeIdx], _msTimeouts[shadeIdx]))
            {
                Log.trace("Timeout\tidx %d\tduration %d", shadeIdx, _msTimeouts[shadeIdx]);
                clearShadeBits(shadeIdx);
                somethingSet = true;
            }
        }
    }
    if (somethingSet)
    {
        setAllOutputs();
    }
}


void WindowShades::doCommand(int shadeIdx, String& cmdStr, String& durationStr)
{
    // Get duration and on/off
    int pinOn      = false;
    int msDuration = 0;

    if (durationStr.equalsIgnoreCase("on"))
    {
        pinOn      = true;
        msDuration = MAX_SHADE_ON_MILLSECS;
    }
    else if (durationStr.equalsIgnoreCase("off"))
    {
        pinOn      = false;
        msDuration = 0;
    }
    else if (durationStr.equalsIgnoreCase("pulse"))
    {
        pinOn      = true;
        msDuration = PULSE_ON_MILLISECS;
    }

    // Handle commands
    if (cmdStr.equalsIgnoreCase("up"))
    {
        setTimedOutput(shadeIdx, SHADE_UP_BIT_MASK, pinOn, msDuration, true);
    }
    else if (cmdStr.equalsIgnoreCase("stop"))
    {
        setTimedOutput(shadeIdx, SHADE_STOP_BIT_MASK, pinOn, msDuration, true);
    }
    else if (cmdStr.equalsIgnoreCase("down"))
    {
        setTimedOutput(shadeIdx, SHADE_DOWN_BIT_MASK, pinOn, msDuration, true);
    }
    else if (cmdStr.equalsIgnoreCase("setuplimit"))
    {
        setTimedOutput(shadeIdx, SHADE_STOP_BIT_MASK | SHADE_DOWN_BIT_MASK, pinOn, msDuration, true);
    }
    else if (cmdStr.equalsIgnoreCase("setdownlimit"))
    {
        setTimedOutput(shadeIdx, SHADE_STOP_BIT_MASK | SHADE_UP_BIT_MASK, pinOn, msDuration, true);
    }
    else if (cmdStr.equalsIgnoreCase("resetmemory"))
    {
        setTimedOutput(shadeIdx, SHADE_STOP_BIT_MASK | SHADE_DOWN_BIT_MASK | SHADE_UP_BIT_MASK, pinOn, msDuration, true);
    }
}
