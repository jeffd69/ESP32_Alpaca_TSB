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

#include <WiFiManager.h>            // https://github.com/tzapu/WiFiManager
#include <SLog.h>
#include <AlpacaDebug.h>
#include <AlpacaServer.h>

#include <Dome.h>
#include <Switch.h>
#include <SafetyMonitor.h>

Dome domeDevice;
Switch switchDevice;
SafetyMonitor safemonDevice;

#define VERSION "1.0.0"

// #define TEST_PROVISIONING

// ASCOM Alpaca server with discovery
AlpacaServer alpaca_server(ALPACA_MNG_SERVER_NAME, ALPACA_MNG_MANUFACTURE, ALPACA_MNG_MANUFACTURE_VERSION, ALPACA_MNG_LOCATION);

uint16_t _shift_reg_in, _shift_reg_out, _prev_shift_reg_out;
bool _dome_open_button, _dome_close_button, _dome_switch_opened, _dome_switch_closed;
bool _dome_relay_open, _dome_relay_close;

uint8_t _safemon_inputs;                  // status of safety monitor 0->safe
uint32_t tmr_rain_ini, tmr_rain_len;      // timers for rain delay and alarm duration
uint32_t tmr_power_ini, tmr_power_len;    // power
uint32_t tmr_wstat_ini, tmr_wstat_len;    // weather station

bool _sw_in[8], _sw_out[8];
uint8_t _sw_pwm[4];
uint8_t _sw_pwm_pins[4] = {OUT_PIN_PWM0, OUT_PIN_PWM1, OUT_PIN_PWM2, OUT_PIN_PWM3};

uint32_t tmr_LED, tmr_shreg;

uint16_t read_shift_register( void );
void write_shift_register( uint16_t value );
void init_IO(void);
void provisioning(void);
void normal_boot(void);

void setup()
{
  pinMode(IN_PIN_AP_SET, INPUT);             // net configuration button (no pullup on chip)
  pinMode(OUT_PIN_AP_LED, OUTPUT);           // net configuration LED
  digitalWrite(OUT_PIN_AP_LED, HIGH);        // turn LED OFF
  delay(100);

  if( LOW == digitalRead(IN_PIN_AP_SET)) {
    digitalWrite(OUT_PIN_AP_LED, LOW);     // turn LED ON
    delay(1000);
    if( LOW == digitalRead(IN_PIN_AP_SET)) {
      Serial.println("Entering WiFi provisioning mode.");
      provisioning();
    }
  }

  normal_boot();
}

void loop()
{
#ifdef TEST_RESTART
  checkForRestart();
#endif

  alpaca_server.Loop();

  domeDevice.Loop();

  switchDevice.Loop();

  safemonDevice.Loop();

  if((millis() - tmr_shreg) > 100) {                  // read shift register every 100ms
    tmr_shreg = millis();
    _shift_reg_in = read_shift_register();
  }

  if( domeDevice.GetNumberOfConnectedClients() > 0 )
  {
    _shift_reg_out |= BIT_DOME;                       // Dome connected LED ON

    _dome_close_button = false;                       // if a client is connected, prevent manual open/close
    _dome_open_button = false;

    if( _shift_reg_in & BIT_FC_CLOSE )                // handle close switch input
      _dome_switch_closed = true;
    else
      _dome_switch_closed = false;

    if( _shift_reg_in & BIT_FC_OPEN )                 // handle open switch input
      _dome_switch_opened = true;
    else 
      _dome_switch_opened = false;

    if( _dome_relay_close )                            // handle close relay bit in the shift register
      _shift_reg_out |= BIT_ROOF_CLOSE;
    else
      _shift_reg_out &= ~BIT_ROOF_CLOSE;
    
    if( _dome_relay_open )                             // handle open relay bit in the shift register
      _shift_reg_out |= BIT_ROOF_OPEN;
    else
      _shift_reg_out &= ~BIT_ROOF_OPEN;

  }
  else
  {
    _shift_reg_out &= ~BIT_DOME;            // Dome connected LED OFF

    // set flags according to bits in shift registers
    if( _shift_reg_in & BIT_BUTTON_CLOSE)   // if no clients connected, handle manual close button
      _dome_close_button = true;
    else
      _dome_close_button = false;
    
    if( _shift_reg_in & BIT_BUTTON_OPEN )   // if no clients connected, handle manual open button
      _dome_open_button = true;
    else
      _dome_open_button = false;
    
    if( _shift_reg_in & BIT_FC_CLOSE )      // handle close switch
      _dome_switch_closed = true;
    else
      _dome_switch_closed = false;

    if( _shift_reg_in & BIT_FC_OPEN )       // handle open switch
      _dome_switch_opened = true;
    else 
      _dome_switch_opened = false;
    
    // set relays if conditions are met
    if( _dome_close_button && !_dome_open_button && !_dome_switch_closed ) {
      _dome_relay_close = true;
      _dome_relay_open = false;
    }
    else if( !_dome_close_button && _dome_open_button && !_dome_switch_opened ) {
      _dome_relay_close = false;
      _dome_relay_open = true;
    } else{
      _dome_relay_close = false;
      _dome_relay_open = false;
    }

    if( _dome_relay_close )                  // set close relay bit in the shift register
      _shift_reg_out |= BIT_ROOF_CLOSE;
    else
      _shift_reg_out &= ~BIT_ROOF_CLOSE;
    
    if( _dome_relay_open )                   // set open relay bit in the shift register
      _shift_reg_out |= BIT_ROOF_OPEN;
    else
      _shift_reg_out &= ~BIT_ROOF_OPEN;
  }

  if( safemonDevice.GetNumberOfConnectedClients() > 0 )
  {
    _shift_reg_out |= BIT_SAFEMON;                    // SafetyMonitor connected LED ON

    if(( _shift_reg_in & BIT_SAFE_RAIN ) != 0) {
      if( tmr_rain_ini != 0 ) {
        tmr_rain_ini = millis();
        tmr_rain_len = 1000 * safemonDevice.getRainDelay();
      }
    } else {
      tmr_rain_ini = 0;
    }

    if(( tmr_rain_ini != 0 ) && ((millis() - tmr_rain_ini) > tmr_rain_len))    // if alarm persists for rain_delay, set UNSAFE
      _safemon_inputs |= SAFEMON_RAIN_BIT;
    else
      _safemon_inputs &= ~SAFEMON_RAIN_BIT;

    if( safemonDevice.getPowerDelay() > 0 ) {                 // enter only if power delay is > 0
      if(( _shift_reg_in & BIT_SAFE_POWER ) != 0) {
        if( tmr_power_ini != 0 ) {
          tmr_power_ini = millis();
          tmr_power_len = 1000 * safemonDevice.getPowerDelay();
        }
      }
      else
      {
        tmr_power_ini = 0;
      }
    
      if((tmr_power_ini != 0) && ((millis() - tmr_power_ini) > tmr_power_len))
        _safemon_inputs |= SAFEMON_POWER_BIT;
      else
        _safemon_inputs &= ~SAFEMON_POWER_BIT;
    }
    else
    {
      _safemon_inputs &= ~SAFEMON_POWER_BIT;
    }
  }
  else
  {
    _shift_reg_out &= ~BIT_SAFEMON;                   // SafetyMonitor connected LED OFF
    _safemon_inputs = 0;
  }

  if( switchDevice.GetNumberOfConnectedClients() > 0)
  {
    uint32_t i;
    uint16_t p;

    _shift_reg_out |= BIT_SWITCH;                 // Switch connected LED ON

    for(i=0; i<8; i++)
    {
      if((_shift_reg_in & (1 << i) != 0))                // set _sw_in[] according to shift register inputs
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

      if( p == 0)                                     // set PWM pin to 0
        digitalWrite(_sw_pwm_pins[i], LOW);
      else if( p == 255)                              // set PWM pin to 1
        digitalWrite(_sw_pwm_pins[i], HIGH);
      else                                            // set PWM value
        analogWrite(_sw_pwm_pins[i], (int)p);
    }
  }
  else
  {
    uint32_t i;

    _shift_reg_out &= ~BIT_SWITCH;                    // Switch connected LED OFF
    _shift_reg_out &= BIT_OUT_CLEAR;                  // clear all OUT bits
    for(i=0; i<8; i++)
    {
      _sw_out[i] = false;                             // clear all out
      _sw_in[i] = false;                              // set input to false
    }

    for(i=0; i<4; i++)
    {
      _sw_pwm[i] = 0;                                 // clear all PWMs
      digitalWrite(_sw_pwm_pins[i], LOW);             // set PWM pin to 0
    }
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

}

// if wifi is not configured, setup WiFiManager to configure via web
void provisioning()
{
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  Serial.begin(115200);

  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  // setup menu to be shown in configuration page
  const char * menu[] = {"wifi","setup","sep","exit"};
  wm.setMenu(menu, sizeof(menu));

  Serial.println("Keep button pressed for 5s to reset WiFi settings.");
  delay(5000);
  if( LOW == digitalRead(IN_PIN_AP_SET))
  {
    Serial.println("Stored AP settings erased!");
  // reset settings - wipe stored credentials that are stored by the esp library (for testing)
    wm.resetSettings();
    for(int i=0; i< 10; i++){
      digitalWrite(OUT_PIN_AP_LED, HIGH);         // turn LED OFF
      delay(50);
      digitalWrite(OUT_PIN_AP_LED, LOW);          // turn LED ON
      delay(50);
    }
  }

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "Alpaca_TSB_AP")
  wm.setConfigPortalTimeout(300);

  bool res;
  res = wm.autoConnect("Alpaca_TSB_AP","password");     // password protected ap

  if(!res) {
    Serial.println("Failed to connect");
    ESP.restart();
  } 
  else {
    //if you get here you have connected to the WiFi    
    Serial.println("connected...yeey :)");
  }

  delay(1000);
  digitalWrite(OUT_PIN_AP_LED, HIGH);        // turn LED OFF
  Serial.println("End of provisioning!");
}

void normal_boot()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("Serial OK");

  init_IO();
  // setup logging and WiFi
  g_Slog.Begin(Serial, 115200);
  SLOG_NOTICE_PRINTF("SLog started\n");
  SLOG_INFO_PRINTF("Try to connect with WiFi\n");

  WiFi.mode(WIFI_STA);
  WiFi.begin(); // (DEFAULT_SSID, DEFAULT_PWD);

  uint16_t _attempts = 0;
  while ((WiFi.status() != WL_CONNECTED) && (_attempts < 60))
  {
    SLOG_INFO_PRINTF("Connecting to WiFi ..\n");
    delay(1000);
    _attempts++;
  }

  if(!(_attempts < 60)) {
    ESP.restart();
  }
  
  IPAddress ip = WiFi.localIP();
  char wifi_ipstr[32]; // = "xxx.yyy.zzz.www";
  snprintf(wifi_ipstr, sizeof(wifi_ipstr), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  SLOG_INFO_PRINTF("connected with %s\n", wifi_ipstr);
  
  // finalize logging setup
  g_Slog.Begin(alpaca_server.GetSyslogHost().c_str());
  SLOG_INFO_PRINTF("SYSLOG enabled and running log_lvl=%s enable_serial=%s\n", g_Slog.GetLvlMskStr().c_str(), alpaca_server.GetSerialLog() ? "true" : "false"); 
  g_Slog.SetLvlMsk(alpaca_server.GetLogLvl());
  g_Slog.SetEnableSerial(alpaca_server.GetSerialLog());

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

  _shift_reg_in = 0;
  _shift_reg_out = 0;
  _prev_shift_reg_out = 0;

  tmr_LED = millis();
  tmr_shreg = millis();

  _safemon_inputs = 0;
  tmr_rain_ini = 0; tmr_rain_len =0 ;
  tmr_power_ini = 0; tmr_power_len = 0; 
  tmr_wstat_ini = 0; tmr_wstat_len = 0;
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
  pinMode(SR_OUT_PIN_OE, OUTPUT);             // output enable
  pinMode(SR_OUT_PIN_STCP, OUTPUT);           // storage clock pulse
  pinMode(SR_OUT_PIN_MR, OUTPUT);             // master reset
  pinMode(SR_OUT_PIN_SHCP, OUTPUT);           // shift register clock pulse
  pinMode(SR_OUT_PIN_SDOUT, OUTPUT);          // serial data out

  pinMode(OUT_PIN_PWM0, OUTPUT);
  pinMode(OUT_PIN_PWM1, OUTPUT);
  pinMode(OUT_PIN_PWM2, OUTPUT);
  pinMode(OUT_PIN_PWM3, OUTPUT);

  pinMode(SR_IN_PIN_CE, OUTPUT);                // chip enable
  pinMode(SR_IN_PIN_CP, OUTPUT);                // clock pulse
  pinMode(SR_IN_PIN_PL, OUTPUT);                // parallel latch
  pinMode(SR_IN_PIN_SDIN, INPUT);               // serial data in

  pinMode(IN_PIN_AP_SET, INPUT_PULLUP);      // net configuration button
  pinMode(OUT_PIN_AP_LED, OUTPUT);           // net configuration LED
  
  digitalWrite(SR_OUT_PIN_OE, LOW);
  digitalWrite(SR_OUT_PIN_STCP, LOW);
  digitalWrite(SR_OUT_PIN_MR, LOW);
  digitalWrite(SR_OUT_PIN_SHCP, LOW);
  digitalWrite(SR_OUT_PIN_SDOUT, LOW);

  digitalWrite(OUT_PIN_PWM0, LOW);
  digitalWrite(OUT_PIN_PWM1, LOW);
  digitalWrite(OUT_PIN_PWM2, LOW);
  digitalWrite(OUT_PIN_PWM3, LOW);

  digitalWrite(SR_IN_PIN_CE, HIGH);
  digitalWrite(SR_IN_PIN_CP, LOW);
  digitalWrite(SR_IN_PIN_PL, HIGH);

  digitalWrite(OUT_PIN_AP_LED, LOW);

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

