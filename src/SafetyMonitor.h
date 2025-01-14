/**************************************************************************************************
  Filename:       SafetyMonitor.h
  Revised:        $Date: 2024-12-10$
  Revision:       $Revision: 01 $
  Description:    Device Alpaca SafetyMonitor V3
**************************************************************************************************/

#pragma once
#include "AlpacaSafetyMonitor.h"

#define SAFEMON_RAIN_BIT        1
#define SAFEMON_POWER_BIT       2

extern u_int8_t _safemon_inputs;

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
  uint32_t getRainDelay() {return _rain_delay;}
  uint32_t getPowerDelay() {return _power_delay;}

};
