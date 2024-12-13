/**************************************************************************************************
  Filename:       main.cpp
  Revised:        Date: 2024-12-02
  Revision:       Revision: 01

  Description:    ASCOM Alpaca ESP32 TSBoard implementation
**************************************************************************************************/
//#define CONFIG_ASYNC_TCP_USE_WDT 0


// #define TEST_RESTART             // only for testing
#include "Credentials.h"
#include "defines.h"                // pins and bitmasks

#include <SLog.h>
#include <AlpacaDebug.h>
#include <AlpacaServer.h>

#include<Dome.h>
#include <Switch.h>
#include <SafetyMonitor.h>

Dome domeDevice;
Switch switchDevice;
SafetyMonitor safemonDevice;

#define VERSION "2.0.1"

// ASCOM Alpaca server with discovery
AlpacaServer alpaca_server(ALPACA_MNG_SERVER_NAME, ALPACA_MNG_MANUFACTURE, ALPACA_MNG_MANUFACTURE_VERSION, ALPACA_MNG_LOCATION);

#ifdef TEST_RESTART
// ========================================================================
// SW Restart
bool restart = false;                          // enable/disable
uint32_t g_restart_start_time_ms = 0xFFFFFFFF; // Timer for countdown
uint32_t const k_RESTART_DELAY_MS = 10000;     // Restart Delay

/**
 * SetRestart
 */
void ActivateRestart()
{
  restart = true;
  g_restart_start_time_ms = millis();
}

/*
 */
void checkForRestart()
{
  if (alpaca_server.GetResetRequest() || restart)
  {
    uint32_t timer_ms = millis() - g_restart_start_time_ms;
    uint32_t coun_down_sec = (k_RESTART_DELAY_MS - timer_ms) / 1000;

    if (timer_ms >= k_RESTART_DELAY_MS)
    {
      ESP.restart();
    }
  }
  else
  {
    g_restart_start_time_ms = millis();
  }
}
#endif

uint16_t shiftreg_in, shiftreg_out;
static bool _do_open_button, _do_close_button, _do_opened_switch, _do_closed_switch;
static bool _do_roof_open, _do_roof_close;

static uint8_t _sa_inputs;

static bool _sw_in[8], _sw_out[8];
static uint8_t _sw_pwm[4];

uint16_t read_shift_register( void );
void write_shift_register( uint16_t value );
void init_IO( void );

void setup()
{
  // setup logging and WiFi
  g_Slog.Begin(Serial, 115200);

  init_IO();

  SLOG_INFO_PRINTF("ESP32ALPACADeviceDemo started ...\n");

  WiFi.mode(WIFI_STA);
  WiFi.begin(DEFAULT_SSID, DEFAULT_PWD);

  while (WiFi.status() != WL_CONNECTED)
  {
    SLOG_INFO_PRINTF("Connecting to WiFi ..\n");
    delay(1000);
  }
  
  IPAddress ip = WiFi.localIP();
  char wifi_ipstr[32] = "xxx.yyy.zzz.www";
  snprintf(wifi_ipstr, sizeof(wifi_ipstr), "%03d.%03d.%03d.%03d", ip[0], ip[1], ip[2], ip[3]);
  SLOG_INFO_PRINTF("connected with %s\n", wifi_ipstr);
  

  // setup ESP32AlpacaDevices
  // 1. Init AlpacaServer
  // 2. Init and add devices
  // 3. Finalize AlpacaServer
  alpaca_server.Begin();

  domeDevice.Begin();
  alpaca_server.AddDevice(&domeDevice);

  switchDevice.Begin();
  alpaca_server.AddDevice(&switchDevice);

  safemonDevice.Begin();
  alpaca_server.AddDevice(&safemonDevice);

  alpaca_server.RegisterCallbacks();
  alpaca_server.LoadSettings();

  // finalize logging setup
  g_Slog.Begin(alpaca_server.GetSyslogHost().c_str());
  SLOG_INFO_PRINTF("SYSLOG enabled and running log_lvl=%s enable_serial=%s\n", g_Slog.GetLvlMskStr().c_str(), alpaca_server.GetSerialLog() ? "true" : "false"); 
  g_Slog.SetLvlMsk(alpaca_server.GetLogLvl());
  g_Slog.SetEnableSerial(alpaca_server.GetSerialLog());

}

void loop()
{
#ifdef TEST_RESTART
  checkForRestart();
#endif

  alpaca_server.Loop();
  delay(25);

  domeDevice.Loop();
  delay(25);

  switchDevice.Loop();
  delay(25);

  safemonDevice.Loop();
  delay(25);


  if( domeDevice.GetNumberOfConnectedClients() > 0 )
  {

  }

  

}


uint16_t read_shift_register( void )
{
  // TODO read inputs from shift register 165
  uint16_t v = 0;

  return v;
}

void write_shift_register( uint16_t value )
{
  // TODO put value on the shift register 595
}

void init_IO( void )
{
  pinMode(SR_OUT_PIN_OE, OUTPUT);       // output enable
  pinMode(SR_OUT_PIN_STCP, OUTPUT);     // storage clock pulse
  pinMode(SR_OUT_PIN_MR, OUTPUT);       // master reset
  pinMode(SR_OUT_PIN_SHCP, OUTPUT);     // shift register clock pulse
  pinMode(SR_OUT_PIN_SDOUT, OUTPUT);    // serial data

  pinMode(SR_OUT_PWM0, OUTPUT);
  pinMode(SR_OUT_PWM1, OUTPUT);
  pinMode(SR_OUT_PWM2, OUTPUT);
  pinMode(SR_OUT_PWM3, OUTPUT);

  pinMode(SR_IN_PIN_CE, OUTPUT);
  pinMode(SR_IN_PIN_CP, OUTPUT);
  pinMode(SR_IN_PIN_PL, OUTPUT);
  pinMode(SR_IN_PIN_SDIN, INPUT);

  pinMode(SR_IN_PIN_AP_SET, INPUT);
  pinMode(SR_OUT_PIN_AP_LED, OUTPUT);
  
  digitalWrite(SR_OUT_PIN_OE, LOW);
  digitalWrite(SR_OUT_PIN_STCP, LOW);
  digitalWrite(SR_OUT_PIN_MR, LOW);
  digitalWrite(SR_OUT_PIN_SHCP, LOW);
  digitalWrite(SR_OUT_PIN_SDOUT, LOW);

  digitalWrite(SR_OUT_PWM0, LOW);
  digitalWrite(SR_OUT_PWM1, LOW);
  digitalWrite(SR_OUT_PWM2, LOW);
  digitalWrite(SR_OUT_PWM3, LOW);

  digitalWrite(SR_IN_PIN_CE, LOW);
  digitalWrite(SR_IN_PIN_CP, LOW);
  digitalWrite(SR_IN_PIN_PL, LOW);

  digitalWrite(SR_OUT_PIN_AP_LED, LOW);

  // clock pulse on 595 shift register with OE and MR low
  digitalWrite(SR_OUT_PIN_SHCP, HIGH);
  usleep(10);
  digitalWrite(SR_OUT_PIN_STCP, HIGH);
  usleep(10);
  digitalWrite(SR_OUT_PIN_SHCP, LOW);
  usleep(10);
  digitalWrite(SR_OUT_PIN_STCP, LOW);
  usleep(10);
  digitalWrite(SR_OUT_PIN_MR, HIGH);

}

