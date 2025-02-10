/**************************************************************************************************
  Filename:       Dome.h
  Revised:        $Date: 2024-12-02$
  Revision:       $Revision: 01 $
  Description:    Device Alpaca Dome
**************************************************************************************************/
#pragma once
#include "AlpacaDome.h"

// ASCOM / ALPACA ShutterStatus Enumeration
/*
enum struct ShutterStatus_t
{
  kOpen = 0,
  kClosed,
  kOpening,
  kClosing,
  kError
};
*/

extern bool d_switch_opened, d_switch_closed;
extern bool d_relay_open, d_relay_close;

class Dome : public AlpacaDome
{
private:

	AlpacaShutterStatus_t d_shutter;		// shutter status
	bool d_slewing;							// true when shutter is moving
	bool d_use_switch;					// if true, use limit switches, else use timeout
	int32_t d_timeout;					// open/close timeout
	int32_t d_timer_ini;					// timer init of movement
	int32_t d_timer_end;					// timer init of movement

	const bool _putAbort();				// to be implemented here
	const bool _putClose();
	const bool _putOpen();
	const AlpacaShutterStatus_t _getShutter();
	const bool _getSlewing();
	void AlpacaReadJson(JsonObject &root);
	void AlpacaWriteJson(JsonObject &root);

	void _dome_use_limit(bool use_lim) { d_use_switch = use_lim; };

	static const char *const k_shutter_state_str[5];

public:
	Dome();
	void Begin();
	void Loop();
};
