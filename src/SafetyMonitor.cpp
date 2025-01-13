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
    if( _safemon_inputs == 0 )
        _is_safe = true;
    else
        _is_safe = false;
    
    return _is_safe;
}

void SafetyMonitor::AlpacaReadJson(JsonObject &root)
{
    DBG_JSON_PRINTFJ(SLOG_NOTICE, root, "SAFEMON READ BEGIN (root=<%s>) ...\n", _ser_json_);
    AlpacaSafetyMonitor::AlpacaReadJson(root);

    if (JsonObject obj_config = root["SafetyMonitor_Configuration"])
    {
        int32_t _rd = obj_config["Rain_delay"] | _rain_delay;
		int32_t _pd = obj_config["Power_off_delay"] | _power_delay;

        if((_rd < 1) || (_rd > 60)){
            _rd = 2;
        }

        if((_pd < 0) || (_pd > 600)){
            _pd = 0;
        }

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
    SLOG_PRINTF(SLOG_NOTICE, "SAFEMON WRITE BEGIN ...\n");
    AlpacaSafetyMonitor::AlpacaWriteJson(root);

    // Config
    JsonObject obj_config = root["SafetyMonitor_Configuration"].to<JsonObject>();
    obj_config["Rain_delay"] = _rain_delay;
	obj_config["Power_off_delay"] = _power_delay;

/*
    // #add # for read only
    JsonObject obj_states = root["SafetyMonitor_States"].to<JsonObject>();
    obj_states["#Is_Safe"] = (_is_safe ? "SAFE" : "UNSAFE");
    obj_states["#Rain_delay"] = _rain_delay;
	obj_states["#Power_off_delay"] = _power_delay;
*/
    DBG_JSON_PRINTFJ(SLOG_NOTICE, root, "... END root=<%s>\n", _ser_json_);
}


