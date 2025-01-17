/**************************************************************************************************
  Filename:       SafetyMonitor.cpp
  Revised:        $Date: 2024-12-10$
  Revision:       $Revision: 01 $
  Description:    Device Alpaca SafetyMonitor V3
**************************************************************************************************/

#include "SafetyMonitor.h"

const char *const k_safemon_state_str[2] = {"Safe", "Unsafe"};

SafetyMonitor::SafetyMonitor() : AlpacaSafetyMonitor()
{
	// constructor
	_is_safe = true;
}

void SafetyMonitor::Begin()
{
	AlpacaSafetyMonitor::Begin();
}

void SafetyMonitor::Loop()
{
	// TODO
	if( _safemon_inputs == 0 )
		_is_safe = true;
	else
		_is_safe = false;
}

const bool SafetyMonitor::_getIsSafe()
{
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

		if((_rd < 1) || (_rd > 60))       // validate dalay on rain signal 1~60s
			_rd = 2;

		if((_pd < 0) || (_pd > 600))      // validate delay on power outage 0~600 (0 means not in use)
			_pd = 0;

		_rain_delay = _rd;
		_power_delay = _pd;

		SLOG_PRINTF(SLOG_INFO, "...SAFEMON READ END _rain_delay=%i _power_delay=%i\n", (int)_rain_delay, (int)_power_delay);

	}

	if (JsonObject obj_ws_config = root["Weather_station"])
	{
		//if( is_ws_connected ) {
			int32_t _tsky = obj_ws_config["Sky_temperature"] | (int32_t)weather_tsky;
			int32_t _wind = obj_ws_config["Wind_speed"] | (int32_t)weather_wind;
			int32_t _hum = obj_ws_config["Humidity"] | (int32_t)weather_hum;
			int32_t _lig = obj_ws_config["Ambient_light"] | (int32_t)weather_light;
/*
			if((_tsky < -50) || (_tsky > 50)) _tsky = 0;
			if((_wind < 0) || (_wind > 100)) _wind = 20;
			if((_hum < 0) || (_hum > 100)) _hum = 95;
			if((_lig < 0) || (_lig > 9999)) _lig = 0;
*/			
			weather_tsky = (int16_t)_tsky;
			weather_wind = (int16_t)_wind;
			weather_hum = (int16_t)_hum;
			weather_light = (int16_t)_lig;

			String _st =(obj_ws_config["Use_sky_temp"] | _st);
			_st.toLowerCase();
			_use_tsky = (_st == "true" ? true : false);

			String _sw =(obj_ws_config["Use_wind"] | _sw);
			_sw.toLowerCase();
			_use_wind = (_sw == "true" ? true : false);

			String _sh =(obj_ws_config["Use_humid"] | _sh);
			_sh.toLowerCase();
			_use_hum = (_sh == "true" ? true : false);

			String _sl =(obj_ws_config["Use_light"] | _sl);
			_sl.toLowerCase();
			_use_light = (_sl == "true" ? true : false);


		//} else {
		//	_use_tsky = false;
		//	_use_wind = false;
		//	_use_hum = false;
		//	_use_light = false;
		//}

	}
	else
	{
		SLOG_PRINTF(SLOG_WARNING, "...SAFEMON READ END no configuration\n");
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


	//if( is_ws_connected ) {
		JsonObject obj_ws_config = root["Weather_Station"].to<JsonObject>();
		obj_ws_config["Sky_temperature"] = (int32_t)weather_tsky;
		obj_ws_config["Use_sky_temp"] = (_use_tsky == true);
		obj_ws_config["Wind_speed"] = (int32_t)weather_wind;
		obj_ws_config["Use_wind"] = (_use_wind == true);
		obj_ws_config["Humidity"] = (int32_t)weather_hum;
		obj_ws_config["Use_humid"] = (_use_hum == true);
		obj_ws_config["Ambient_light"] = (int32_t)weather_light;
		obj_ws_config["Use_light"] = (_use_light == true);
	//}

/*
	// #add # for read only
	JsonObject obj_states = root["SafetyMonitor_States"].to<JsonObject>();
	obj_states["#Is_Safe"] = (_is_safe ? "SAFE" : "UNSAFE");
	obj_states["#Rain_delay"] = _rain_delay;
	obj_states["#Power_off_delay"] = _power_delay;
*/
	DBG_JSON_PRINTFJ(SLOG_NOTICE, root, "...SAFEMON WRITE END root=<%s>\n", _ser_json_);
}


