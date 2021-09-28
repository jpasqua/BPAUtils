#ifndef Output_h
#define Output_h

#include <TimeLib.h>

namespace Output {
  void setOptions(bool* metric, bool* use24Hour);

  // ----- Units
  float temp(float temp);
  float tempSpread(float spread);
  float baro(float baro);
  const char* tempUnits();
  const char* baroUnits();

  // ----- Time
  String formattedTime(bool includeSeconds = false);
  String formattedTime(time_t theTime, bool includeSeconds = false);
  String formattedInterval(int h, int m, int s, bool zeroPadHours = false, bool includeSeconds = true);
  String formattedInterval(uint32_t seconds, bool zeroPadHours = true, bool includeSeconds = true);
};

#endif  // Output_h