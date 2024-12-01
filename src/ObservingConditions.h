/**************************************************************************************************
  Filename:       ObservingConditions.h
  Revised:        Date: 2024-12-02
  Revision:       Revision: 01

  Description:    ASCOM Alpaca ESP32 TSBoard implementation
**************************************************************************************************/
#pragma once
#include "AlpacaObservingConditions.h"

class ObservingConditions : public AlpacaObservingConditions
{
public:
  ObservingConditions();
  void Begin();
  void Loop();
  
private:
  uint32_t _update_time_ms = 0;
  uint32_t _refresh_time_ms = 1000;

  // virtual methods
  void _putRefreshRequest() { _update_time_ms = 0; };
  const bool _putAveragePeriodRequest(double average_period);

  void _refresh();

  void AlpacaReadJson(JsonObject &root);
  void AlpacaWriteJson(JsonObject &root);
};