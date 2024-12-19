/**************************************************************************************************
  Filename:       SafetyMonitor.cpp
  Revised:        $Date: 2024-12-10$
  Revision:       $Revision: 01 $
  Description:    Device Alpaca SafetyMonitor V3
**************************************************************************************************/

#include "SafetyMonitor.h"

SafetyMonitor::SafetyMonitor() : AlpacaSafetyMonitor()
{
    // constructor
}

void SafetyMonitor::Begin()
{
    AlpacaSafetyMonitor::Begin();
}

void SafetyMonitor::Loop()
{
    // TODO
}

const bool SafetyMonitor::_getIsSafe()
{
    return _is_safe;
}

void SafetyMonitor::AlpacaReadJson(JsonObject &root)
{
    DBG_JSON_PRINTFJ(SLOG_NOTICE, root, "BEGIN (root=<%s>) ...\n", _ser_json_);
    AlpacaSafetyMonitor::AlpacaReadJson(root);

    if (JsonObject obj_config = root["SafetyMonitorConfiguration"])
    {
        int32_t _rd = obj_config["Rain delay"] | _rain_delay;
		int32_t _pd = obj_config["Power off delay"] | _power_delay;

        _rain_delay = _rd;
        _power_delay = _pd;

        SLOG_PRINTF(SLOG_INFO, "... END  _rain_delay=%i _power_delay=%i\n", (int)_rain_delay, (int)_power_delay);
    }
    else
    {
        SLOG_PRINTF(SLOG_WARNING, "... END no configuration\n");
    }
}

void SafetyMonitor::AlpacaWriteJson(JsonObject &root)
{
    SLOG_PRINTF(SLOG_NOTICE, "BEGIN ...\n");
    AlpacaSafetyMonitor::AlpacaWriteJson(root);

    // Config
    JsonObject obj_config = root["SafetyMonitorConfiguration"].to<JsonObject>();
    obj_config["Rain delay"] = _rain_delay;
	obj_config["Power off delay"] = _power_delay;

    // #add # for read only
    JsonObject obj_states = root["#States"].to<JsonObject>();
    obj_states["Is Safe"] = (_is_safe ? "SAFE" : "UNSAFE");
    obj_states["Rain delay"] = _rain_delay;
	obj_states["Power off delay"] = _power_delay;

    DBG_JSON_PRINTFJ(SLOG_NOTICE, root, "... END root=<%s>\n", _ser_json_);
}


