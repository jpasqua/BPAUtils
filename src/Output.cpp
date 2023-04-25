#include <BPABasics.h>
#include <Output.h>

namespace Output {
  // ----- Options
  static bool MetricDefault = false;
  static bool Use24HourDefault = false;
  static bool* _useMetric = &MetricDefault;
  static bool* _use24Hour = &Use24HourDefault;

  void setOptions(bool* useMetric, bool* use24Hour) {
    _use24Hour = use24Hour;
    _useMetric = useMetric;
  }
  
  bool using24HourMode() { return _use24Hour; }
  bool usingMetric() { return _useMetric; }

  // ----- Units
  static const char* MetricTempUnits = "°C";
  static const char* ImperialTempUnits = "°F";
  static const char* MetricBaroUnits = "hPa";
  static const char* ImperialBaroUnits = "inHg";


  float temp(float temp) { return (*_useMetric) ? temp : Basics::c_to_f(temp); }
  float tempSpread(float spread) { return (*_useMetric) ? spread : Basics::delta_c_to_f(spread); }
  float baro(float baro) { return (*_useMetric) ? baro : Basics::hpa_to_inhg(baro); }
  const char* tempUnits() { return (*_useMetric) ? MetricTempUnits : ImperialTempUnits; }
  const char* baroUnits() { return (*_useMetric) ? MetricBaroUnits : ImperialBaroUnits; }


  // ----- Time
  int adjustedHour(int h24) {
    if (*_use24Hour) return h24;
    if (h24 == 0) return 12;
    if (h24 > 12) return h24 - 12;
    return h24;
  }

  String formattedDateTime(time_t theTime, bool includeSeconds, bool zeroPadHours) {
    char buf[] = "2023-03-13";
    sprintf(buf, "%4d-%02d-%02d ", year(theTime), month(theTime), day(theTime));
    String dateTime(buf);

    String formattedTime = formattedInterval(
        *_use24Hour ? hour(theTime) : hourFormat12(theTime),
        minute(theTime), second(theTime), zeroPadHours, includeSeconds);

    dateTime.concat(formattedTime);
    return dateTime;
  }

  String formattedTime(time_t theTime, bool includeSeconds, bool zeroPadHours) {
    return formattedInterval(
        *_use24Hour ? hour(theTime) : hourFormat12(theTime),
        minute(theTime), second(theTime), zeroPadHours, includeSeconds);
  }

  String formattedTime(bool includeSeconds, bool zeroPadHours) {
    return formattedTime(now(), includeSeconds, zeroPadHours);
  }

  String formattedInterval(int h, int m, int s, bool zeroPadHours, bool includeSeconds) {
    // Return a result of the form hh:mm:ss (seconds are optional)
    String result;
    result.reserve(5 + includeSeconds ? 3 : 0);
    if (zeroPadHours && (h < 10))  result += "0"; result += h;
    result += ':';
    if (m < 10)  result += "0"; result += m; 
    if (includeSeconds) {
      result += ':';
      if (s < 10)  result += "0"; result += s;
    }
    return result;
  }

  String formattedInterval(uint32_t seconds, bool zeroPadHours, bool includeSeconds) {
    int h = seconds / Basics::SecondsPerHour;
    int m = (seconds / Basics::SecondsPerMinute) % Basics::SecondsPerMinute;
    int s = (seconds % Basics::SecondsPerMinute);       
    return formattedInterval(h, m, s, zeroPadHours, includeSeconds);
  }
};
