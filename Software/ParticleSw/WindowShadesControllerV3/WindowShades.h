// WindowShades
// Rob Dobson 2013-2017
// More details at http://robdobson.com/2013/10/moving-my-window-shades-control-to-mbed/

#pragma once

#include "application.h"

class WindowShades
{
public:
    static const long PULSE_ON_MILLISECS    = 500;
    static const long MAX_SHADE_ON_MILLSECS = 60000;
    static const int  SHADE_UP_BIT_MASK     = 1;
    static const int  SHADE_STOP_BIT_MASK   = 2;
    static const int  SHADE_DOWN_BIT_MASK   = 4;
    static const int  BITS_PER_SHADE        = 3;
    static const int  MAX_WINDOW_SHADES     = 6;


    WindowShades(int hc595_SER, int hc595_SCK, int hc595_RCK);

    void service();
    void doCommand(int shadeIdx, String& cmdStr, String& durationStr);

    int getMaxNumShades()
    {
        return MAX_WINDOW_SHADES;
    }


private:
    int           _hc595_SER;
    int           _hc595_SCK;
    int           _hc595_RCK;
    unsigned long _msTimeouts[MAX_WINDOW_SHADES];
    int           _tickCounts[MAX_WINDOW_SHADES];

    int _curShadeCtrlBits;

    void clearShadeBits(int shadeIdx);
    void setAllOutputs();
    void setTimedOutput(int shadeIdx, int bitMask, bool bitOn, long msDuration, bool bClearExisting);
    void setShadeBit(int shadeIdx, int bitMask, int bitIsOn);
};
