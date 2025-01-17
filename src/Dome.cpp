/**************************************************************************************************
  Filename:       Dome.cpp
  Revised:        Date: 2024-12-02
  Revision:       Revision: 01

  Description:    Dome Device implementation
**************************************************************************************************/
#include "Dome.h"

const char *const Dome::k_shutter_state_str[5] = {"Open", "Closed", "Opening", "Closing", "Error"};

Dome::Dome() : AlpacaDome()
{
	// constructor
}

void Dome::Begin()
{
    // init Dome
    AlpacaDome::Begin();

	// init shutter status
	if( _use_switch )
	{
		if( _dome_switch_closed )
			_shutter = AlpacaShutterStatus_t::kClosed;
		else if( _dome_switch_opened )
			_shutter = AlpacaShutterStatus_t::kOpen;
		else
			_shutter = AlpacaShutterStatus_t::kError;
	}
	else
	{
		_shutter = AlpacaShutterStatus_t::kError;
	}
}

void Dome::Loop()
{
    // simulation of roof moving and changing status using timer (no limit switches)

	if( _use_switch )
	{
		if(( _shutter == AlpacaShutterStatus_t::kOpening ) || ( _shutter == AlpacaShutterStatus_t::kClosing )) {
			if(( millis() - _timer_ini ) > (_timeout * 1000 ))		// timeout!!!!!!!!!!!
			{
				SLOG_WARNING_PRINTF("WARNING! Dome timeout!");
				_shutter = AlpacaShutterStatus_t::kError;			// set error status
				_slew = false;
				_timer_ini = 0;
				_timer_end = 0;
				_dome_relay_close = false;							// turn relays OFF
				_dome_relay_open = false;
				return;
			}
		}

		if(( _shutter == AlpacaShutterStatus_t::kOpening ) && ( _dome_switch_opened ))
		{
			SLOG_INFO_PRINTF("Dome open.");
			_shutter == AlpacaShutterStatus_t::kOpen;
			_slew = false;
			_dome_relay_close = false;		// turn relays OFF
			_dome_relay_open = false;
		}
		
		if(( _shutter == AlpacaShutterStatus_t::kClosing ) && ( _dome_switch_closed ))
		{
			SLOG_INFO_PRINTF("Dome closed.");
			_shutter == AlpacaShutterStatus_t::kClosed;
			_slew = false;
			_dome_relay_close = false;		// turn relays OFF
			_dome_relay_open = false;
		}
	}
	else
	{
		if(( _shutter == AlpacaShutterStatus_t::kOpening ) && ( millis() > _timer_end ))
		{
			_shutter == AlpacaShutterStatus_t::kOpen;
			_slew = false;
			_dome_relay_close = false;		// turn relays OFF
			_dome_relay_open = false;
		}
		
		if(( _shutter == AlpacaShutterStatus_t::kClosing ) && ( millis() > _timer_end ))
		{
			_shutter == AlpacaShutterStatus_t::kClosed;
			_slew = false;
			_dome_relay_close = false;		// turn relays OFF
			_dome_relay_open = false;
		}
	}
}

const bool Dome::_putAbort()	// stops shutter motor, sets _shutter to error, set _slew to false
{
	SLOG_INFO_PRINTF("Dome Halted.");
    _shutter = AlpacaShutterStatus_t::kError;
	_slew = false;
	
	_timer_ini = 0;
	_timer_end = 0;

	_dome_relay_close = false;		// turn relays OFF
	_dome_relay_open = false;
	
	return true;
}

const bool Dome::_putClose()
{
    if( _shutter == AlpacaShutterStatus_t::kOpening )
	{
		SLOG_WARNING_PRINTF("WARNING! Dome close command while opening");
		return false;
	}
	else if( _shutter == AlpacaShutterStatus_t::kClosing )
	{
		SLOG_INFO_PRINTF("INFO Dome is already closing. Command ignored.");
		return true;
	}
	else
	{
		SLOG_INFO_PRINTF("Dome command close received.");
		_slew = true;
		_shutter = AlpacaShutterStatus_t::kClosing;
		
		_timer_ini = millis();
		_timer_end = _timer_ini + _timeout * 1000;

		_dome_relay_close = true;		// turn close relays ON
		_dome_relay_open = false;		// turn open relays OFF
	}
	
	return true;
}

const bool Dome::_putOpen()
{
    if( _shutter == AlpacaShutterStatus_t::kClosing )
	{
		SLOG_WARNING_PRINTF("WARNING! Dome open command while closing");
		return false;
	}
	else if( _shutter == AlpacaShutterStatus_t::kOpening )
	{
		SLOG_INFO_PRINTF("INFO Dome is already opening. Command ignored.");
		return true;
	}
	else
	{
		SLOG_INFO_PRINTF("Dome command open received.");
		_slew = true;
		_shutter = AlpacaShutterStatus_t::kOpening;
		
		_timer_ini = millis();
		_timer_end = _timer_ini + _timeout * 1000;

		_dome_relay_close = false;		// turn close relays OFF
		_dome_relay_open = true;			// turn open relays ON
	}
	
	return true;
}

const AlpacaShutterStatus_t Dome::_getShutter()
{
    return _shutter;
}

const bool Dome::_getSlewing()
{
    return _slew;
}

// read settings from flash
void Dome::AlpacaReadJson(JsonObject &root)
{
	DBG_JSON_PRINTFJ(SLOG_NOTICE, root, "DOME READ BEGIN (root=<%s>) ...\n", _ser_json_);
	AlpacaDome::AlpacaReadJson(root);

	if (JsonObject obj_config = root["Dome_Configuration"])
	{
		String _str =(obj_config["Use_limit_switches"] | _str);
		//Serial.print("########## AlpacaRead "); Serial.print(_str); Serial.println(" ########## ");
		// _use_switch =(obj_config["Use_limit_switches"] | (_use_switch == true));

		uint32_t _to = obj_config["Shutter_timeout"] | _timeout;
		if((_to < 1) || (_to > 300)) {	// validate 0~300s
			_to = 60;
		}
		
		//Serial.print("AlpacaRead "); Serial.println(_str);
		_str.toLowerCase();
		_use_switch = (_str == "true" ? true : false);
		_timeout = _to;
/*
		bool _sus =(obj_config["Test_switches"] | (_use_switch == true));
		Serial.print("########## "); Serial.print(_sus); Serial.println(" ########## ");

		uint32_t _us = (obj_config["Use_limit_switches"] | 1) == 0 ? false : true;
		if((_us < 0) || (_us > 1)) {	// validate 0 -> not in use, 1 -> switches in use
			_us = 0;
		}

		uint32_t _to = obj_config["Shutter_timeout"] | _timeout;
		if((_to < 1) || (_to > 300)) {	// validate 0~300s
			_to = 60;
		}
		
		_use_switch = (_us == 0); // ? false : true;
		_timeout = _to;
*/

		SLOG_PRINTF(SLOG_INFO, "...DOME READ  END  _use_switch=%s _timeout=%i\n", (_use_switch ? "true" : "false"), _timeout);
	}
	else
	{
		SLOG_PRINTF(SLOG_WARNING, "...DOME READ  END no configuration\n");
	}
}

// persist settings to flash
void Dome::AlpacaWriteJson(JsonObject &root)
{
    SLOG_PRINTF(SLOG_NOTICE, "DOME WRITE BEGIN ...\n");
    AlpacaDome::AlpacaWriteJson(root);

    // Config
    JsonObject obj_config = root["Dome_Configuration"].to<JsonObject>();
	obj_config["Use_limit_switches"] = (_use_switch == true);
    obj_config["Shutter_timeout"] = _timeout;

	Serial.print("AlpacaWrite "); Serial.println(_use_switch);

	/*
	obj_config["Test_switches"] = (true == _use_switch);
	obj_config["Use_limit_switches"] = _use_switch ? 1 : 0;
    obj_config["Shutter_timeout"] = _timeout;
	*/
    DBG_JSON_PRINTFJ(SLOG_NOTICE, root, "...DOME WRITE END root=<%s>\n", _ser_json_);
}