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

extern bool _dome_open_button, _dome_close_button, _dome_opened_switch, _dome_closed_switch;
extern bool _dome_roof_open, _dome_roof_close;

class Dome : public AlpacaDome
{
private:

	AlpacaShutterStatus_t _shutter;		// shutter status
	bool _slew;							// true when shutter is moving
	bool _use_switch;					// if true, use limit switches, else use timeout
	int32_t _timeout;					// open/close timeout
	int32_t _overclose;					// extra close time if _use_switch is true
	int32_t _timer_ini;					// timer init of movement
	int32_t _timer_end;					// timer init of movement

	const bool _putAbort();				// to be implemented here
	const bool _putClose();
	const bool _putOpen();
	const AlpacaShutterStatus_t _getShutter();
	const bool _getSlewing();
	void AlpacaReadJson(JsonObject &root);
	void AlpacaWriteJson(JsonObject &root);

	static const char *const k_shutter_state_str[5];

public:
	Dome();
	void Begin();
	void Loop();
};
