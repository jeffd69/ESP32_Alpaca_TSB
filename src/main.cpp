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

uint16_t _shift_reg_in, _shift_reg_out, _prev_shift_reg_out;
bool _dome_open_button, _dome_close_button, _dome_opened_switch, _dome_closed_switch;
bool _dome_roof_open, _dome_roof_close;

uint8_t _safemon_inputs;

bool _sw_in[8], _sw_out[8];
uint8_t _sw_pwm[4];
uint8_t _sw_pwm_pins[4] = {SR_OUT_PWM0, SR_OUT_PWM1, SR_OUT_PWM2, SR_OUT_PWM3};

uint32_t tmr_LED, tmr_shreg;

uint16_t read_shift_register( void );
void write_shift_register( uint16_t value );
void init_IO( void );

void setup()
{
  Serial.begin(115200);
  while(!Serial);

  Serial.println("Serial OK");

  // setup logging and WiFi
  g_Slog.Begin(Serial, 115200);
  SLOG_NOTICE_PRINTF("SLogDemo started\n");

  SLOG_INFO_PRINTF("Try to connect with WFi %s\n", DEFAULT_SSID);

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

  g_Slog.Begin(String(SYSLOG_HOST), 514); //  (String(SYSLOG_HOST), 514);
  

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

  _shift_reg_in = 0;
  _shift_reg_out = 0;
  _prev_shift_reg_out = 0;

  tmr_LED = millis();
  tmr_shreg = millis();
}

void loop()
{
#ifdef TEST_RESTART
  checkForRestart();
#endif

  alpaca_server.Loop();
  delay(1);

  domeDevice.Loop();
  delay(1);

  switchDevice.Loop();
  delay(1);

  safemonDevice.Loop();
  delay(1);

  if((millis() - tmr_shreg) > 100)                    // read shift register every 100ms
    _shift_reg_in = read_shift_register();

  if( domeDevice.GetNumberOfConnectedClients() > 0 )
  {
    _shift_reg_out |= BIT_DOME;             // Dome connected LED ON

    _dome_close_button = false;             // if a client is connected, prevent manual open/close
    _dome_open_button = false;

    if( _shift_reg_in & BIT_FC_CLOSE )      // handle close switch input
      _dome_closed_switch = true;
    else
      _dome_closed_switch = false;

    if( _shift_reg_in & BIT_FC_OPEN )       // handle open switch input
      _dome_opened_switch = true;
    else 
      _dome_opened_switch = false;

    if( _dome_roof_close )                  // handle close relay bit in the shift register
      _shift_reg_out |= BIT_ROOF_CLOSE;
    else
      _shift_reg_out &= ~BIT_ROOF_CLOSE;
    
    if( _dome_roof_open )                   // handle open relay bit in the shift register
      _shift_reg_out |= BIT_ROOF_OPEN;
    else
      _shift_reg_out &= ~BIT_ROOF_OPEN;

  }
  else
  {
    _shift_reg_out &= ~BIT_DOME;            // Dome connected LED OFF

    if( _shift_reg_in & BIT_BUTTON_CLOSE)   // if no clients connected, handle manual close button
      _dome_close_button = true;
    else
      _dome_close_button = false;
    
    if( _shift_reg_in & BIT_BUTTON_OPEN )   // if no clients connected, handle manual open button
      _dome_open_button = true;
    else
      _dome_open_button = false;
    
    if( _shift_reg_in & BIT_FC_CLOSE )      // handle close switch
      _dome_closed_switch = true;
    else
      _dome_closed_switch = false;

    if( _shift_reg_in & BIT_FC_OPEN )       // handle open switch
      _dome_opened_switch = true;
    else 
      _dome_opened_switch = false;
    
    if( _dome_roof_close )                  // set close relay bit in the shift register
      _shift_reg_out |= BIT_ROOF_CLOSE;
    else
      _shift_reg_out &= ~BIT_ROOF_CLOSE;
    
    if( _dome_roof_open )                   // set open relay bit in the shift register
      _shift_reg_out |= BIT_ROOF_OPEN;
    else
      _shift_reg_out &= ~BIT_ROOF_OPEN;
  }

  if( switchDevice.GetNumberOfConnectedClients() > 0)
  {
    uint32_t i;
    uint16_t p;

    _shift_reg_out |= BIT_SWITCH;                 // Switch connected LED ON

    for(i=0; i<8; i++)
    {
      if((_shift_reg_in & i) != 0)                // set _sw_in[] according to shift register inputs
        _sw_in[i] = true;
      else
        _sw_in[i] = false;
    
      if( _sw_out[i] )                            // set out bits according to _sw_out[] status
        _shift_reg_out |= (BIT_OUT_0 >> i);
      else
        _shift_reg_out &= ~(BIT_OUT_0 >> i);
    }

    for(i=0; i<4; i++)
    {
      p = ((uint16_t)_sw_pwm[i] * 255) / 100;

      if( p == 0)                                 // set PWM pin to 0
        digitalWrite(_sw_pwm_pins[i], LOW);
      else if( p == 255)                          // set PWM pin to 1
        digitalWrite(_sw_pwm_pins[i], HIGH);
      else                                        // set PWM value
        analogWrite(_sw_pwm_pins[i], (int)p);
    }
  }
  else
  {
    uint32_t i;

    _shift_reg_out &= ~BIT_SWITCH;                // Switch connected LED OFF

    for(i=0; i<8; i++)
    {
      _shift_reg_out &= ~(BIT_OUT_0 >> i);        // clear relay bits
      _sw_in[i] = false;                          // set input to false
    }

    for(i=0; i<4; i++)
    {
      digitalWrite(_sw_pwm_pins[i], LOW);         // set PWM pin to 0
    }
  }

  if( safemonDevice.GetNumberOfConnectedClients() > 0 )
  {
    _shift_reg_out |= BIT_SAFEMON;                    // SafetyMonitor connected LED ON

    _safemon_inputs = (( _shift_reg_in & BIT_SAFE_RAIN ) == 0 ) ? 0 : 1;
    _safemon_inputs += (( _shift_reg_in & BIT_SAFE_POWER ) == 0 ) ? 0 : 2;

  }
  else
  {
    _shift_reg_out &= ~BIT_SAFEMON;                   // SafetyMonitor connected LED OFF
    _safemon_inputs = 0;
  }

  if(( millis() - tmr_LED ) < 1000 )                  // blink CPU OK LED
  {
    if(( millis() - tmr_LED ) < 500 )
      _shift_reg_out |= BIT_CPU_OK;
    else
      _shift_reg_out &= ~BIT_CPU_OK;
  }
  else
  {
    tmr_LED = millis();
  }

  if((millis() - tmr_shreg) > 100)                    // write shift register every 100ms
  {
    tmr_shreg = millis();

    if( _shift_reg_out != _prev_shift_reg_out )       // write only if changed
    {
      _prev_shift_reg_out = _shift_reg_out;
      write_shift_register( _shift_reg_out );
    }
  }
}

// read inputs from shift register 165, returns uint16_t value
uint16_t read_shift_register( void )
{
  uint16_t v = 0;

  digitalWrite(SR_IN_PIN_CP, LOW);    // be sure CP is low
  digitalWrite(SR_IN_PIN_PL, LOW);    // latch parallel inputs
  delayMicroseconds(1);
  digitalWrite(SR_IN_PIN_PL, HIGH);
  delayMicroseconds(1);
  digitalWrite(SR_IN_PIN_CE, LOW);    // on CE -> low, D7 is available on serial out Q7
  delayMicroseconds(1);

  for(uint16_t i=0; i<16; i++) {
    v = (v << 1);
    v += digitalRead(SR_IN_PIN_SDIN);

    digitalWrite(SR_IN_PIN_CP, HIGH);   // shift to the left
    delayMicroseconds(1);
    digitalWrite(SR_IN_PIN_CP, LOW);
    delayMicroseconds(1);
  }

  digitalWrite(SR_IN_PIN_CE, HIGH);

  return v;
}

// put value on the shift registers 595
void write_shift_register( uint16_t value )
{
  digitalWrite(SR_OUT_PIN_SDOUT, LOW);        // 14 serial data low
  digitalWrite(SR_OUT_PIN_MR, LOW);           // 10 clear previous data
  delayMicroseconds(1);
  digitalWrite(SR_OUT_PIN_SHCP, HIGH);        // 11 shift register clock
  delayMicroseconds(1);
  digitalWrite(SR_OUT_PIN_SHCP, LOW);
  delayMicroseconds(1);
  digitalWrite(SR_OUT_PIN_MR, HIGH);
  delayMicroseconds(1);

  for(uint8_t i = 0; i < 16; i++) {
    if((value & 0x8000) == 0 )
      digitalWrite(SR_OUT_PIN_SDOUT, LOW);
    else
      digitalWrite(SR_OUT_PIN_SDOUT, HIGH);
    
    delayMicroseconds(1);
    digitalWrite(SR_OUT_PIN_SHCP, HIGH);
    delayMicroseconds(1);
    digitalWrite(SR_OUT_PIN_SHCP, LOW);

    value = (value << 1);
  }

  delayMicroseconds(1);
  digitalWrite(SR_OUT_PIN_STCP, HIGH);        // transfer serial data to parallel output
  delayMicroseconds(1);
  digitalWrite(SR_OUT_PIN_STCP, LOW);         // 12
  digitalWrite(SR_OUT_PIN_OE, LOW);           // 13 enable output
}

void init_IO( void )
{
  pinMode(SR_OUT_PIN_OE, OUTPUT);       // output enable
  pinMode(SR_OUT_PIN_STCP, OUTPUT);     // storage clock pulse
  pinMode(SR_OUT_PIN_MR, OUTPUT);       // master reset
  pinMode(SR_OUT_PIN_SHCP, OUTPUT);     // shift register clock pulse
  pinMode(SR_OUT_PIN_SDOUT, OUTPUT);    // serial data out

  pinMode(SR_OUT_PWM0, OUTPUT);
  pinMode(SR_OUT_PWM1, OUTPUT);
  pinMode(SR_OUT_PWM2, OUTPUT);
  pinMode(SR_OUT_PWM3, OUTPUT);

  pinMode(SR_IN_PIN_CE, OUTPUT);        // chip enable
  pinMode(SR_IN_PIN_CP, OUTPUT);        // clock pulse
  pinMode(SR_IN_PIN_PL, OUTPUT);        // parallel latch
  pinMode(SR_IN_PIN_SDIN, INPUT);       // serial data in

  pinMode(SR_IN_PIN_AP_SET, INPUT);     // net configuration button
  pinMode(SR_OUT_PIN_AP_LED, OUTPUT);   // net configuration LED
  
  digitalWrite(SR_OUT_PIN_OE, LOW);
  digitalWrite(SR_OUT_PIN_STCP, LOW);
  digitalWrite(SR_OUT_PIN_MR, LOW);
  digitalWrite(SR_OUT_PIN_SHCP, LOW);
  digitalWrite(SR_OUT_PIN_SDOUT, LOW);

  digitalWrite(SR_OUT_PWM0, LOW);
  digitalWrite(SR_OUT_PWM1, LOW);
  digitalWrite(SR_OUT_PWM2, LOW);
  digitalWrite(SR_OUT_PWM3, LOW);

  digitalWrite(SR_IN_PIN_CE, HIGH);
  digitalWrite(SR_IN_PIN_CP, LOW);
  digitalWrite(SR_IN_PIN_PL, HIGH);

  digitalWrite(SR_OUT_PIN_AP_LED, LOW);

  // clock pulse on 74HC595 shift register with OE and MR low
  digitalWrite(SR_OUT_PIN_SHCP, HIGH);
  usleep(10);
  digitalWrite(SR_OUT_PIN_STCP, HIGH);
  usleep(10);
  digitalWrite(SR_OUT_PIN_SHCP, LOW);
  usleep(10);
  digitalWrite(SR_OUT_PIN_STCP, LOW);
  usleep(10);
  digitalWrite(SR_OUT_PIN_MR, HIGH);

  analogWriteFrequency(1000);         // set PWM 1KHz 8bits
  analogWriteResolution(8);
}

