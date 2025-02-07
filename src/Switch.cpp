/**************************************************************************************************
  Filename:       Switch.cpp
  Revised:        Date: 2024-12-02
  Revision:       Revision: 01

  Description:    ASCOM Alpaca ESP32 TSBoard implementation
**************************************************************************************************/
#include "Switch.h"

const uint32_t k_num_of_switch_devices = 20;

SwitchDevice_t init_switch_device[k_num_of_switch_devices] = {
    {false, false, "Switch_0", "IN 1 (R)", 0.0, 0.0, 1.0, 1.0},
    {false, false, "Switch_1", "IN 2 (R)", 0.0, 0.0, 1.0, 1.0},
    {false, false, "Switch_2", "IN 3 (R)", 0.0, 0.0, 1.0, 1.0},
    {false, false, "Switch_3", "IN 4 (R)", 0.0, 0.0, 1.0, 1.0},
    {false, false, "Switch_4", "IN 5 (R)", 0.0, 0.0, 1.0, 1.0},
    {false, false, "Switch_5", "IN 6 (R)", 0.0, 0.0, 1.0, 1.0},
    {false, false, "Switch_6", "IN 7 (R)", 0.0, 0.0, 1.0, 1.0},
    {false, false, "Switch_7", "IN 8 (R)", 0.0, 0.0, 1.0, 1.0},
    {false, true, "Switch_8", "OUT 1 (RW)", 0.0, 0.0, 1.0, 1.0},
    {false, true, "Switch_9", "OUT 2 (RW)", 0.0, 0.0, 1.0, 1.0},
    {false, true, "Switch_10", "OUT 3 (RW)", 0.0, 0.0, 1.0, 1.0},
    {false, true, "Switch_11", "OUT 4 (RW)", 0.0, 0.0, 1.0, 1.0},
    {false, true, "Switch_12", "OUT 5 (RW)", 0.0, 0.0, 1.0, 1.0},
    {false, true, "Switch_13", "OUT 6 (RW)", 0.0, 0.0, 1.0, 1.0},
    {false, true, "Switch_14", "OUT 7 (RW)", 0.0, 0.0, 1.0, 1.0},
    {false, true, "Switch_15", "OUT 8 (RW)", 0.0, 0.0, 1.0, 1.0},

    {false, true, "Switch_16", "PWM 1 (RW)", 0.0, 0.0, 100.0, 1.0},
    {false, true, "Switch_17", "PWM 2 (RW)", 0.0, 0.0, 100.0, 1.0},
    {false, true, "Switch_18", "PWM 3 (RW)", 0.0, 0.0, 100.0, 1.0},
    {false, true, "Switch_19", "PWM 4 (RW)", 0.0, 0.0, 100.0, 1.0}
    };

Switch::Switch() : AlpacaSwitch(k_num_of_switch_devices)
{
  // constructor
  //_p_swtc = AlpacaSwitch::_p_switch_devices;
}

void Switch::Begin()
{
  for (uint32_t u = 0; u < k_num_of_switch_devices; u++)
  {
    InitSwitchInitBySetup(u, init_switch_device[u].init_by_setup);
    InitSwitchCanWrite(u, init_switch_device[u].can_write);
    InitSwitchName(u, init_switch_device[u].name);
    InitSwitchDescription(u, init_switch_device[u].description);
    InitSwitchValue(u, init_switch_device[u].value);
    InitSwitchMinValue(u, init_switch_device[u].min_value);
    InitSwitchMaxValue(u, init_switch_device[u].max_value);
    InitSwitchStep(u, init_switch_device[u].step);
  }

  AlpacaSwitch::Begin();

  // SLOG_PRINTF(SLOG_INFO, "REGISTER handler for \"%s\"\n", "/setup/v1/switch/0/setup");
  // _p_alpaca_server->getServerTCP()->on("/setup/v1/switch/0/setup", HTTP_GET, [this](AsyncWebServerRequest *request)
  //                                      { DBG_REQ; _alpacaGetPage(request, FOCUSER_SETUP_URL); DBG_END; });

  // Init switches
  // TODO

#ifdef DEBUG_SWITCH
  DebugSwitchDevice(k_num_of_switch_devices);
#endif
}

void Switch::Loop()
{
  // copy inputs to AlpacaSwitch::_p_switch_devices
  for(int i=0; i<8; i++)
  {
    if(_sw_in[i])                             // set input value to AlpacaSwitch::_p_switch_devices[] array. Value is read from shift register
      AlpacaSwitch::SetSwitch(i, true);
    else
      AlpacaSwitch::SetSwitch(i, false);
  
    if( AlpacaSwitch::GetValue(i + 8) )       // set OUTs and PWMs to HW
      _sw_out[i] = true;
    else
      _sw_out[i] = false;
  }

  for(int i=0; i<4; i++)
  {
    _sw_pwm[i + 16] = (uint8_t)AlpacaSwitch::GetSwitchValue(i + 16);
  }
}

/**
 * This methode is called by AlpacaSwitch to manipulate physical device
 */
const bool Switch::_writeSwitchValue(uint32_t id, double value)
{
  // TODO write to physical device, GPIO, etc
  bool result = false; // wrong id or invalid value

  // TODO check id
  if(id < 8) {
    SLOG_WARNING_PRINTF("WARNING. Attempt to write to a read-only switch.")
    return false;
  }

  if(id > (k_num_of_switch_devices-1)) {
    SLOG_WARNING_PRINTF("WARNING. Invalid switch ID.")
    return false;
  }

  if((id > 7 ) && ( id < 16 ))
    _sw_out[id - 8] = (value != 0 ? true : false);
  else
    _sw_pwm[id - 16] = (uint8_t)value;

#ifdef DEBUG_SWITCH
  DebugSwitchDevice(id);
#endif
  result = true;
  SLOG_DEBUG_PRINTF("id=%d value=%f result=%s", id, value, result ? "true" : "false");

  return result;
}

// read settings from flash
void Switch::AlpacaReadJson(JsonObject &root)
{
	DBG_JSON_PRINTFJ(SLOG_NOTICE, root, "SWITCH READ BEGIN (root=<%s>) ...\n", _ser_json_);
	AlpacaSwitch::AlpacaReadJson(root);

	char title[32] = "";
  if (JsonObject obj_config = root["Switch_Configuration"])
  {
    char sw_name[kSwitchNameSize] = "";
    for (uint32_t u = 0; u < GetMaxSwitch(); u++)
    {
      snprintf(sw_name, sizeof(sw_name), "Ch_%d", u);
      InitSwitchName(u, obj_config[sw_name] | GetSwitchName(u));
      DBG_JSON_PRINTFJ(SLOG_NOTICE, obj_config, "... title=%s obj_config=<%s> \n", sw_name, _ser_json_);
    }
  }
	SLOG_PRINTF(SLOG_NOTICE, "...SWITCH READ END\n");
}

// persist settings to flash
void Switch::AlpacaWriteJson(JsonObject &root)
{
  DBG_JSON_PRINTFJ(SLOG_NOTICE, root, "SWITCH WRITE BEGIN root=%s ...\n", _ser_json_);
  AlpacaSwitch::AlpacaWriteJson(root);

  // prepare Config
  JsonObject obj_config = root["Switch_Configuration"].to<JsonObject>();
  char sw_name[kSwitchNameSize] = "";
  for (uint32_t u = 0; u < GetMaxSwitch(); u++)
  {
    snprintf(sw_name, sizeof(sw_name), "Ch_%d", u);
    obj_config[sw_name] = (String)GetSwitchName(u);
    DBG_JSON_PRINTFJ(SLOG_NOTICE, obj_config, "... title=%s obj_config=<%s> \n", sw_name, _ser_json_);
  }
  DBG_JSON_PRINTFJ(SLOG_NOTICE, root, "...SWITCH WRITE END \"%s\"\n", _ser_json_);
}

/* ORIGINAL VERSION FROM PETER
void Switch::AlpacaReadJson(JsonObject &root)
{
	DBG_JSON_PRINTFJ(SLOG_NOTICE, root, "BEGIN (root=<%s>) ...\n", _ser_json_);
	AlpacaSwitch::AlpacaReadJson(root);

	char title[32] = "";
	for (uint32_t u = 0; u < GetMaxSwitch(); u++)
	{
		if (GetSwitchInitBySetup(u))
		{
			snprintf(title, sizeof(title), "Configuration_Device_%d", u);
			if (JsonObject obj_config = root[title])
			{
				InitSwitchName(u, obj_config["Name"] | GetSwitchName(u));
				InitSwitchDescription(u, obj_config["Description"] | GetSwitchDescription(u));
				InitSwitchCanWrite(u, obj_config["CanWrite"] | GetSwitchCanWrite(u));
				InitSwitchMinValue(u, obj_config["MinValue"] | GetSwitchMinValue(u));
				InitSwitchMaxValue(u, obj_config["MaxValue"] | GetSwitchMaxValue(u));
				InitSwitchStep(u, obj_config["Step"] | GetSwitchStep(u));
				DBG_JSON_PRINTFJ(SLOG_NOTICE, obj_config, "... title=%s obj_config=<%s> \n", title, _ser_json_);
			}
		}
	}
	SLOG_PRINTF(SLOG_NOTICE, "... END\n");
}

void Switch::AlpacaWriteJson(JsonObject &root)
{
  DBG_JSON_PRINTFJ(SLOG_NOTICE, root, "BEGIN root=%s ...\n", _ser_json_);
  AlpacaSwitch::AlpacaWriteJson(root);

  char title[32] = "";

  // prepare Config
  for (uint32_t u = 0; u < GetMaxSwitch(); u++)
  {
    if (GetSwitchInitBySetup(u))
    {
      snprintf(title, sizeof(title), "Configuration_Device_%d", u);
      JsonObject obj_config = root[title].to<JsonObject>();
      {
        char s[128];
        snprintf(s, sizeof(s), "%s", GetSwitchName(u));
        obj_config["Name"] = s;
      }
      {
        char s[128];
        snprintf(s, sizeof(s), "%s", GetSwitchDescription(u));
        obj_config["Description"] = s;
      }
      obj_config["CanWrite"] = GetSwitchCanWrite(u);
      obj_config["MinValue"] = GetSwitchMinValue(u);
      obj_config["MaxValue"] = GetSwitchMaxValue(u);
      obj_config["Step"] = GetSwitchStep(u);

      DBG_JSON_PRINTFJ(SLOG_NOTICE, obj_config, "... title=%s (obj_config=<%s>)\n", title, _ser_json_);
    }
  }

  // Prepare states
  for (uint32_t u = 0; u < GetMaxSwitch(); u++)
  {
    // #add # for read only
    snprintf(title, sizeof(title), "#States_Device_%d", u);
    JsonObject obj_state = root[title].to<JsonObject>();
    if (obj_state)
    {
      if (!GetSwitchInitBySetup(u))
      {
        {
          char s[128];
          snprintf(s, sizeof(s), "%s", GetSwitchName(u));
          obj_state["Name"] = s;
        }
        {
          char s[128];
          snprintf(s, sizeof(s), "%s", GetSwitchDescription(u));
          obj_state["Description"] = s;
        }
        obj_state["CanWrite"] = GetSwitchCanWrite(u);
        obj_state["Value"] = GetSwitchValue(u);
        obj_state["MinValue"] = GetSwitchMinValue(u);
        obj_state["MaxValue"] = GetSwitchMaxValue(u);
        obj_state["Step"] = GetSwitchStep(u);
      }
      obj_state["Value"] = GetSwitchValue(u);
      DBG_JSON_PRINTFJ(SLOG_NOTICE, obj_state, "... title=%s (obj_state=<%s>)\n", title, _ser_json_);
    }
  }

  DBG_JSON_PRINTFJ(SLOG_NOTICE, root, "... END \"%s\"\n", _ser_json_);
}
*/

#ifdef DEBUG_SWITCH
/**
 * Log Switch Device data
 * id = 0..k_num_of_switch_devices-1  - log device <id>
 * id = k_num_of_switch_devices       - log all devices
 */
void Switch::DebugSwitchDevice(uint32_t id)
{
  uint32_t tmp_id = 0;
  uint32_t tmp_max = 0;

  if (tmp_id == k_num_of_switch_devices)
  {
    tmp_id = 0;
    tmp_max = k_num_of_switch_devices;
  }
  else
  {
    tmp_id = (id < k_num_of_switch_devices) ? id : 0;
    tmp_max = tmp_id + 1;
  }

  for (uint32_t u = tmp_id; u < tmp_max; u++)
  {
    SLOG_DEBUG_PRINTF("device_id=%d init_by_setup=%s can_write=%s name=%s description=%s value=%lf min_value=%lf max_value=%lf step=%lf\n",
                      u,
                      GetSwitchInitBySetup(u) ? "true" : "false",
                      GetSwitchCanWrite(u) ? "true" : "false",
                      GetSwitchName(u),
                      GetSwitchDescription(u),
                      GetSwitchValue(u),
                      GetSwitchMinValue(u),
                      GetSwitchStep(u)
                      );
  }
}
#endif