/**************************************************************************************************
  Filename:       Dome.cpp
  Revised:        Date: 2024-12-02
  Revision:       Revision: 01

  Description:    Dome Device implementation
**************************************************************************************************/
#include "Dome.h"

const char *const Dome::k_shutter_state_str[5] = {"OPEN", "CLOSED", "OPENING", "CLOSING", "ERROR"};

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
		if( _dome_closed_switch )
			_shutter = AlpacaShutterStatus_t::kClosed;
		else if( _dome_opened_switch )
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
		if(( millis() - _timer_ini ) > (_timeout * 1000 ))		// timeout!!!!!!!!!!!
		{
			_shutter = AlpacaShutterStatus_t::kError;			// set error status
			_slew = false;
			_timer_ini = 0;
			_timer_end = 0;
			_dome_roof_close = false;							// turn relays OFF
			_dome_roof_open = false;
			return;
		}

		if(( _shutter == AlpacaShutterStatus_t::kOpening ) && ( _dome_opened_switch ))
		{
			_shutter == AlpacaShutterStatus_t::kOpen;
			_slew = false;
			_dome_roof_close = false;		// turn relays OFF
			_dome_roof_open = false;
		}
		
		if(( _shutter == AlpacaShutterStatus_t::kClosing ) && ( _dome_closed_switch ))
		{
			_shutter == AlpacaShutterStatus_t::kClosed;
			_slew = false;
			_dome_roof_close = false;		// turn relays OFF
			_dome_roof_open = false;
		}
	}
	else
	{
		if(( _shutter == AlpacaShutterStatus_t::kOpening ) && ( millis() > _timer_end ))
		{
			_shutter == AlpacaShutterStatus_t::kOpen;
			_slew = false;
			_dome_roof_close = false;		// turn relays OFF
			_dome_roof_open = false;
		}
		
		if(( _shutter == AlpacaShutterStatus_t::kClosing ) && ( millis() > _timer_end ))
		{
			_shutter == AlpacaShutterStatus_t::kClosed;
			_slew = false;
			_dome_roof_close = false;		// turn relays OFF
			_dome_roof_open = false;
		}
	}
}

const bool Dome::_putAbort()	// stops shutter motor, sets _shutter to error, set _slew to false
{
    _shutter = AlpacaShutterStatus_t::kError;
	_slew = false;
	
	_timer_ini = 0;
	_timer_end = 0;

	_dome_roof_close = false;		// turn relays OFF
	_dome_roof_open = false;
	
	return true;
}

const bool Dome::_putClose()
{
    if( _shutter == AlpacaShutterStatus_t::kOpening )
	{
		return false;
	}
	else if( _shutter == AlpacaShutterStatus_t::kClosing )
	{
		return true;
	}
	else
	{
		_slew = true;
		_shutter = AlpacaShutterStatus_t::kClosing;
		
		_timer_ini = millis();
		_timer_end = _timer_ini + _timeout * 1000;

		_dome_roof_close = true;		// turn close relays ON
		_dome_roof_open = false;		// turn open relays OFF
	}
	
	return true;
}

const bool Dome::_putOpen()
{
    if( _shutter == AlpacaShutterStatus_t::kClosing )
	{
		return false;
	}
	else if( _shutter == AlpacaShutterStatus_t::kOpening )
	{
		return true;
	}
	else
	{
		_slew = true;
		_shutter = AlpacaShutterStatus_t::kOpening;
		
		_timer_ini = millis();
		_timer_end = _timer_ini + _timeout * 1000;

		_dome_roof_close = false;		// turn close relays OFF
		_dome_roof_open = true;			// turn open relays ON
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


void Dome::AlpacaReadJson(JsonObject &root)
{
	DBG_JSON_PRINTFJ(SLOG_NOTICE, root, "DOME READ BEGIN (root=<%s>) ...\n", _ser_json_);
	AlpacaDome::AlpacaReadJson(root);

	if (JsonObject obj_config = root["DomeConfiguration"])
	{
		//bool _us = obj_config["Use_limit_switches"] | _use_switch;
		_use_switch = obj_config["Use_limit_switches"] | _use_switch;
		_timeout = obj_config["Shutter_timeout"] | _timeout;
		
		// InitSwitchCanWrite(u, obj_config["CanWrite"] | GetSwitchCanWrite(u));
		//_dome_use_limit(obj_config["Use_limit_switches"] | _use_switch);

		//_use_switch = _us;
		//_timeout = _to;

		SLOG_PRINTF(SLOG_INFO, "... END  _use_switch=%s _timeout=%i\n", (_use_switch ? "true" : "false"), _timeout);
	}
	else
	{
		SLOG_PRINTF(SLOG_WARNING, "... END no configuration\n");
	}
}

void Dome::AlpacaWriteJson(JsonObject &root)
{
    SLOG_PRINTF(SLOG_NOTICE, "DOME WRITE BEGIN ...\n");
    AlpacaDome::AlpacaWriteJson(root);

    // Config
    JsonObject obj_config = root["DomeConfiguration"].to<JsonObject>();
    obj_config["Use_limit_switches"] = _use_switch;
    obj_config["Shutter_timeout"] = _timeout;

    // add # before the name for read only
    JsonObject obj_states = root["States"].to<JsonObject>();
    obj_states["#Shutter"] = k_shutter_state_str[(int)_shutter];
	obj_states["#Use_limit_switches"] = _use_switch; //(_use_switch ? "true" : "false");
    obj_states["#Shutter_timeout"] = _timeout;

    DBG_JSON_PRINTFJ(SLOG_NOTICE, root, "... END root=<%s>\n", _ser_json_);
}