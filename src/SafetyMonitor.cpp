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
	if( is_ws_connected ) {
		if( _use_tsky ) {
			if( weather_tsky > _tsky_limit ) {
				if(tmr_ws_sky_ini == 0) {
					tmr_ws_sky_ini = millis();
					tmr_ws_sky_len = _weather_delay * 1000;
				}

				if(( weather_tsky > _tsky_limit ) && (tmr_ws_sky_ini != 0) && ((millis() - tmr_ws_sky_ini) > tmr_ws_sky_len))
					_safemon_inputs |= SAFEMON_TSKY_BIT;
			}
			else
			{
				_safemon_inputs &= ~SAFEMON_TSKY_BIT;
				tmr_ws_sky_ini = 0;
			}
		} else {
			_safemon_inputs &= ~SAFEMON_TSKY_BIT;
			tmr_ws_sky_ini = 0;
		}

		if( _use_wind ) {
			if( weather_wind > _wind_limit ) {
				if(tmr_ws_wind_ini == 0) {
					tmr_ws_wind_ini = millis();
					tmr_ws_wind_len = _weather_delay * 1000;
				}

				if(( weather_wind > _wind_limit ) && (tmr_ws_wind_ini != 0) && ((millis() - tmr_ws_wind_ini) > tmr_ws_wind_len))
					_safemon_inputs |= SAFEMON_WIND_BIT;
			}
			else
			{
				_safemon_inputs &= ~SAFEMON_WIND_BIT;
				tmr_ws_wind_ini = 0;
			}
		} else {
			_safemon_inputs &= ~SAFEMON_WIND_BIT;
			tmr_ws_wind_ini = 0;
		}

	} else {
		_safemon_inputs &= 0x3;						// mask all weather bits
		tmr_ws_sky_ini = 0;
		tmr_ws_wind_ini = 0;
	}
	
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
	bool _valid;

	if (JsonObject obj_config = root["SafetyMonitor_Configuration"])
	{
		uint32_t _rd = obj_config["Rain_delay"] | _rain_delay;
		uint32_t _pd = obj_config["Power_off_delay"] | _power_delay;
		uint32_t _wd = obj_config["Weather_delay"] | _weather_delay;

		String _tsky = obj_config["Sky_temp_limit"] | _tsky;
		String _wind = obj_config["Wind_limit"] | _wind;
		//String _hum = obj_config["Humidity"] | _hum;
		//String _lig = obj_config["Ambient_light"] | _lig;

		if((_rd < 2) || (_rd > 60))       	// validate dalay on rain signal 2~60s
			_rd = 2;

		if((_pd < 0) || (_pd > 600))      	// validate delay on power outage 0~600 (0 means not in use)
			_pd = 0;

		if((_wd < 0) || (_wd > 600))		// validate delay for weather station (0 means not in use)
			_wd = 10;
		
		_rain_delay = _rd;
		_power_delay = _pd;
		_weather_delay = _wd;

		if(_tsky.isEmpty()) {
			_use_tsky = false;
		} else {
			_valid = true;

			for(int i=0; i<_tsky.length(); i++) {
				char c =_tsky.charAt(i);
				if(!(( c == '-') || (c > 0x2f) && (c < 0x3a))) {		// only minus sign and numbers allowed
					_valid = false;
				}
			}

			if( _valid ) {								// not empty string and valid number
				int16_t _ws = (int16_t)_tsky.toInt();
				if(!((_ws < -50) || (_ws > 50))) {
					_use_tsky = true;
					_tsky_limit = _ws;
				}
			} else {
				_use_tsky = false;
			}
		}

		if(_wind.isEmpty()) {
			_use_wind = false;
		} else {
			_valid = true;

			for(int i=0; i< _wind.length(); i++) {
				char c =_wind.charAt(i);
				if((c < 0x30) || (c > 0x39)) {		// only positive numbers allowed
					_valid = false;
				}
			}

			if( _valid ) {								// not empty string and valid number
				int16_t _wi = (int16_t)_wind.toInt();
				if(!((_wi < 0) || (_wi > 100))) {
					_use_wind = true;
					_wind_limit = _wi;
				}
			} else {
				_use_wind = false;
			}		
		}

		char _msg[256];
		snprintf(_msg, sizeof(_msg), "ReadJson tsky limit %i, tsky in use %s, wind limit %i, wind in use %s", _tsky_limit, _use_tsky ? "Yes" : "No", _wind_limit, _use_wind ? "Yes" : "No");
		Serial.println(_msg);

		SLOG_PRINTF(SLOG_INFO, "...SAFEMON READ END _rain_delay=%i _power_delay=%i\n", (int)_rain_delay, (int)_power_delay);
	} else {
		SLOG_PRINTF(SLOG_WARNING, "...SAFEMON READ END no configuration\n");
	}
}

void SafetyMonitor::AlpacaWriteJson(JsonObject &root)
{
	SLOG_PRINTF(SLOG_NOTICE, "SAFEMON WRITE BEGIN ...\n");
	AlpacaSafetyMonitor::AlpacaWriteJson(root);
	char buff[16];

	// Config
	JsonObject obj_config = root["SafetyMonitor_Configuration"].to<JsonObject>();
	obj_config["Rain_delay"] = _rain_delay;
	obj_config["Power_off_delay"] = _power_delay;
	obj_config["Weather_delay"] = _weather_delay;

	if(_use_tsky == true){
		snprintf(buff, sizeof(buff), "%i", _tsky_limit);
		obj_config["Sky_temp_limit"] = buff;
	} else {
		obj_config["Sky_temp_limit"] = "";
	}

	if(_use_wind == true) {
		snprintf(buff, sizeof(buff), "%i", _wind_limit);
		obj_config["Wind_limit"] = buff;
	} else {
		obj_config["Wind_limit"] = "";
	}

	char _msg[256];
	snprintf(_msg, sizeof(_msg), "WriteJson tsky limit %i, tsky in use %s, wind limit %i, wind in use %s", _tsky_limit, _use_tsky ? "Yes" : "No", _wind_limit, _use_wind ? "Yes" : "No");
	Serial.println(_msg);

	/*
	obj_config["Sky_temperature"] = weather_tsky;
	obj_config["Use_sky_temp"] = (_use_tsky == true);
	obj_config["Wind_speed"] = weather_wind;
	obj_config["Use_wind"] = (_use_wind == true);
	obj_config["Humidity"] = weather_hum;
	obj_config["Use_humid"] = (_use_hum == true);
	obj_config["Ambient_light"] = weather_light;
	obj_config["Use_light"] = (_use_light == true);
	*/
	DBG_JSON_PRINTFJ(SLOG_NOTICE, root, "...SAFEMON WRITE END root=<%s>\n", _ser_json_);
}


/*
void SafetyMonitor::AlpacaReadJson(JsonObject &root)
{
	DBG_JSON_PRINTFJ(SLOG_NOTICE, root, "SAFEMON READ BEGIN (root=<%s>) ...\n", _ser_json_);
	AlpacaSafetyMonitor::AlpacaReadJson(root);

	if (JsonObject obj_config = root["SafetyMonitor_Configuration"])
	{
		uint32_t _rd = obj_config["Rain_delay"] | _rain_delay;
		uint32_t _pd = obj_config["Power_off_delay"] | _power_delay;

		if((_rd < 1) || (_rd > 60))       // validate dalay on rain signal 1~60s
			_rd = 2;

		if((_pd < 0) || (_pd > 600))      // validate delay on power outage 0~600 (0 means not in use)
			_pd = 0;

		_rain_delay = _rd;
		_power_delay = _pd;

		//SLOG_PRINTF(SLOG_INFO, "...SAFEMON READ END _rain_delay=%i _power_delay=%i\n", (int)_rain_delay, (int)_power_delay);

	}

	if (JsonObject obj_ws_config = root["Weather_station"])
	{
		//if( is_ws_connected ) {
			int16_t _tsky = obj_ws_config["Sky_temperature"] | weather_tsky;
			int16_t _wind = obj_ws_config["Wind_speed"] | weather_wind;
			int16_t _hum = obj_ws_config["Humidity"] | weather_hum;
			int16_t _lig = obj_ws_config["Ambient_light"] | weather_light;

			if((_tsky < -50) || (_tsky > 50)) _tsky = 0;
			if((_wind < 0) || (_wind > 100)) _wind = 20;
			if((_hum < 0) || (_hum > 100)) _hum = 95;
			if((_lig < 0) || (_lig > 9999)) _lig = 0;
			
			weather_tsky = _tsky;
			weather_wind = _wind;
			weather_hum = _hum;
			weather_light = _lig;

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


	// #add # for read only
	JsonObject obj_states = root["SafetyMonitor_States"].to<JsonObject>();
	obj_states["#Is_Safe"] = (_is_safe ? "SAFE" : "UNSAFE");
	obj_states["#Rain_delay"] = _rain_delay;
	obj_states["#Power_off_delay"] = _power_delay;

	DBG_JSON_PRINTFJ(SLOG_NOTICE, root, "...SAFEMON WRITE END root=<%s>\n", _ser_json_);
}
*/