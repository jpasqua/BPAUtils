#ifndef Output_h
#define Output_h

#include <Arduino.h>
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
  int adjustedHour(int h24);
  String formattedTime(bool includeSeconds = false, bool zeroPadHours = false);
  String formattedTime(time_t theTime, bool includeSeconds = false, bool zeroPadHours = false);
  String formattedInterval(int h, int m, int s, bool zeroPadHours = false, bool includeSeconds = true);
  String formattedInterval(uint32_t seconds, bool zeroPadHours = true, bool includeSeconds = true);
  String formattedDateTime(time_t theTime, bool includeSeconds = false, bool zeroPadHours = true);
};

#endif  // Output_h