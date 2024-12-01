/**************************************************************************************************
  Filename:       Switch.h
  Revised:        Date: 2024-12-02
  Revision:       Revision: 01

  Description:    ASCOM Alpaca ESP32 TSBoard implementation
**************************************************************************************************/
#pragma once
#include "AlpacaSwitch.h"

// comment/uncomment to enable/disable debugging
// #define DEBUG_SWITCH

class Switch : public AlpacaSwitch
{
private:
    const bool _writeSwitchValue(uint32_t id, double value);

    void AlpacaReadJson(JsonObject &root);
    void AlpacaWriteJson(JsonObject &root);

#ifdef DEBUG_SWITCH
    void DebugSwitchDevice(uint32_t id);
#endif    

public:
    Switch();
    void Begin();
    void Loop();
};