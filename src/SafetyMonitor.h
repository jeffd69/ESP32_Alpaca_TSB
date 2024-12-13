/**************************************************************************************************
  Filename:       SafetyMonitor.h
  Revised:        $Date: 2024-12-10$
  Revision:       $Revision: 01 $
  Description:    Device Alpaca SafetyMonitor V3
**************************************************************************************************/

#pragma once
#include "AlpacaSafetyMonitor.h"

extern u_int8_t _sa_inputs;

class SafetyMonitor : public AlpacaSafetyMonitor
{
private:
    bool _is_safe;
    uint32_t _rain_delay;
    uint32_t _power_delay;

    const bool _getIsSafe();

    void AlpacaReadJson(JsonObject &root);
	void AlpacaWriteJson(JsonObject &root);

	static const char *const k_safemon_state_str[2];

public:
	SafetyMonitor();
	void Begin();
	void Loop();

};
