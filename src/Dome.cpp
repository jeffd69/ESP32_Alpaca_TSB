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
	if( d_use_switch )
	{
		if( d_switch_closed )
			d_shutter = AlpacaShutterStatus_t::kClosed;
		else if( d_switch_opened )
			d_shutter = AlpacaShutterStatus_t::kOpen;
		else
			d_shutter = AlpacaShutterStatus_t::kError;
	}
	else
	{
		d_shutter = AlpacaShutterStatus_t::kError;
	}
}

void Dome::Loop()
{
	if( d_use_switch ) {
		if(( d_shutter == AlpacaShutterStatus_t::kOpening ) || ( d_shutter == AlpacaShutterStatus_t::kClosing )) {
			if(( millis() - d_timer_ini ) > (d_timeout * 1000 ))		// timeout!!!!!!!!!!!
			{
				SLOG_ERROR_PRINTF("ERROR! Dome timeout!");
				d_shutter = AlpacaShutterStatus_t::kError;			// set error status
				d_slewing = false;
				d_timer_ini = 0;
				d_timer_end = 0;
				d_relay_close = false;							// turn relays OFF
				d_relay_open = false;
				return;
			}
		}

		if(( d_shutter == AlpacaShutterStatus_t::kOpening ) && ( d_switch_opened ))
		{
			d_shutter == AlpacaShutterStatus_t::kOpen;
			d_slewing = false;
			d_relay_close = false;		// turn relays OFF
			d_relay_open = false;
			SLOG_INFO_PRINTF("Dome open.");
		}
		
		if(( d_shutter == AlpacaShutterStatus_t::kClosing ) && ( d_switch_closed ))
		{
			d_shutter == AlpacaShutterStatus_t::kClosed;
			d_slewing = false;
			d_relay_close = false;		// turn relays OFF
			d_relay_open = false;
			SLOG_INFO_PRINTF("Dome closed.");
		}
	} else {
		if(( d_shutter == AlpacaShutterStatus_t::kOpening ) && ( millis() > d_timer_end ))
		{
			d_shutter == AlpacaShutterStatus_t::kOpen;
			d_slewing = false;
			d_relay_close = false;		// turn relays OFF
			d_relay_open = false;
			SLOG_INFO_PRINTF("Dome open.");
		}
		
		if(( d_shutter == AlpacaShutterStatus_t::kClosing ) && ( millis() > d_timer_end ))
		{
			d_shutter == AlpacaShutterStatus_t::kClosed;
			d_slewing = false;
			d_relay_close = false;		// turn relays OFF
			d_relay_open = false;
			SLOG_INFO_PRINTF("Dome closed.");
		}
	}
}

const bool Dome::_putAbort()	// stops shutter motor, sets _shutter to error, set _slew to false
{
    d_shutter = AlpacaShutterStatus_t::kError;
	d_slewing = false;
	d_timer_ini = 0;
	d_timer_end = 0;
	d_relay_close = false;		// turn relays OFF
	d_relay_open = false;
	SLOG_INFO_PRINTF("Dome Halted.");
	
	return true;
}

const bool Dome::_putClose()
{
    if( d_shutter == AlpacaShutterStatus_t::kOpening ) {
		SLOG_WARNING_PRINTF("WARNING! Dome close command ignored while opening");
		return false;
	} else if( d_shutter == AlpacaShutterStatus_t::kClosing ) {
		SLOG_INFO_PRINTF("INFO Dome is already closing. Command ignored.");
		return true;
	} else {
		d_slewing = true;
		d_shutter = AlpacaShutterStatus_t::kClosing;
		d_timer_ini = millis();
		d_timer_end = d_timer_ini + d_timeout * 1000;

		d_relay_close = true;		// turn close relays ON
		d_relay_open = false;		// turn open relays OFF
		SLOG_INFO_PRINTF("Dome command close received.");
	}
	
	return true;
}

const bool Dome::_putOpen()
{
    if( d_shutter == AlpacaShutterStatus_t::kClosing ) {
		SLOG_WARNING_PRINTF("WARNING! Dome open command ignored while closing");
		return false;
	} else if( d_shutter == AlpacaShutterStatus_t::kOpening ) {
		SLOG_INFO_PRINTF("INFO Dome is already opening. Command ignored.");
		return true;
	} else {
		SLOG_INFO_PRINTF("Dome command open received.");
		d_slewing = true;
		d_shutter = AlpacaShutterStatus_t::kOpening;
		d_timer_ini = millis();
		d_timer_end = d_timer_ini + d_timeout * 1000;

		d_relay_close = false;		// turn close relays OFF
		d_relay_open = true;			// turn open relays ON
	}
	
	return true;
}

const AlpacaShutterStatus_t Dome::_getShutter()
{
    return d_shutter;
}

const bool Dome::_getSlewing()
{
    return d_slewing;
}

// read settings from flash
void Dome::AlpacaReadJson(JsonObject &root)
{
	DBG_JSON_PRINTFJ(SLOG_NOTICE, root, "DOME READ BEGIN (root=<%s>) ...\n", _ser_json_);
	AlpacaDome::AlpacaReadJson(root);

	if (JsonObject obj_config = root["Dome_Configuration"]) {
		String _str =(obj_config["Use_limit_switches"] | _str);
		uint32_t _to = obj_config["Shutter_timeout"] | d_timeout;
		
		if((_to < 1) || (_to > 300)) {	// validate 0~300s
			_to = 60;
		}
		
		_str.toLowerCase();
		d_use_switch = (_str == "true" ? true : false);
		d_timeout = _to;

		SLOG_PRINTF(SLOG_INFO, "...DOME READ END  _use_switch=%s _timeout=%i\n", (d_use_switch ? "true" : "false"), d_timeout);
	} else {
		SLOG_PRINTF(SLOG_WARNING, "...DOME READ END no configuration\n");
	}
}

// persist settings to flash
void Dome::AlpacaWriteJson(JsonObject &root)
{
    SLOG_PRINTF(SLOG_NOTICE, "DOME WRITE BEGIN ...\n");
    AlpacaDome::AlpacaWriteJson(root);

    // Config
    JsonObject obj_config = root["Dome_Configuration"].to<JsonObject>();
	obj_config["Use_limit_switches"] = (d_use_switch == true);
    obj_config["Shutter_timeout"] = d_timeout;

	Serial.print("AlpacaWrite "); Serial.println(d_use_switch);
    DBG_JSON_PRINTFJ(SLOG_NOTICE, root, "...DOME WRITE END root=<%s>\n", _ser_json_);
}