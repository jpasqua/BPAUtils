#ifndef Basics_h
#define Basics_h
#define Basics_h_Version 0.3


#include <Arduino.h>
#include <functional>
#include <TimeLib.h>

// ----- Low-level Macros
#define ALLOC(t,n) (t *) malloc((n)*sizeof(t))
#define REALLOC(p, t,n) (t *) realloc(p, (n)*sizeof(t))
#define FREE(p) { if (p) free(p); }
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

template <typename T, std::size_t N>
constexpr std::size_t countof(T const (&)[N]) noexcept { return N; }

namespace Basics {
  // ----- Basic Types
  using Pin = int;
  using ActionID = uint8_t;
  constexpr Pin UnusedPin = -1;
  constexpr ActionID UnusedAction = 255;

  using StringMapper = std::function<String(const String&)>;
  using ReferenceMapper = std::function<void(const String&, String&)> ;
  using FloatValCB = std::function<void(float)>;

  // ----- String Related
  extern char* newFromString(String& source);

  // ----- Time Related
  // Workaround issue in TimeLib (https://github.com/PaulStoffregen/Time/issues/154)
  #undef minutesToTime_t
  #undef hoursToTime_t
  #undef daysToTime_t
  #undef weeksToTime_t

  #define minutesToTime_t(M) ( (M) * SECS_PER_MIN)  
  #define hoursToTime_t(H)   ( (H) * SECS_PER_HOUR)  
  #define daysToTime_t(D)    ( (D) * SECS_PER_DAY)
  #define weeksToTime_t(W)   ( (W) * SECS_PER_WEEK)

  // --- Constants
  constexpr uint32_t  MillisPerSecond = 1000L;
  constexpr uint32_t  SecondsPerMinute = 60;
  constexpr uint32_t  MinutesPerHour = 60;
  constexpr uint32_t  SecondsPerHour = (SecondsPerMinute * MinutesPerHour);
  constexpr uint32_t  MillisPerHour = (SecondsPerHour * MillisPerSecond);
  constexpr uint32_t  MillisPerMinute = (SecondsPerMinute * MillisPerSecond);

  constexpr uint32_t  minutesToMS(uint32_t m) { return (minutesToTime_t(m) * 1000L); }
  constexpr uint32_t  hoursToMS(uint32_t h) { return (hoursToTime_t(h) * 1000L); }
  constexpr uint32_t  daysToMS(uint32_t d) { return (daysToTime_t(d) * 1000L); }
  constexpr uint32_t  weeksToMS(uint32_t w) { return (weeksToTime_t(w) * 1000L); }

  // Functions
  inline time_t wallClockFromMillis(uint32_t milliTime) {
    return (now() - (millis() - milliTime)/1000L);
  }

  // String Utilities
  inline void resetString(String& target) {
    if (target.length()) target.setCharAt(0, 0); // In case c_str() will be used
    target.clear();
  }


  // ----- Unit conversions

  // Temperature
  inline float c_to_f(float c) { return (c * 9.0f/5.0f) + 32.0f; }
  inline float f_to_c(float f) { return (f - 32.0f) * 9.0f/5.0f; }
  inline float k_to_c(float k) { return k - 273.15f; }
  inline float c_to_k(float c) { return c + 273.15f; }
  inline float delta_c_to_f(float c) { return (c * 9.0f/5.0f);  }
  inline float delta_f_to_c(float f) { return (f * 5.0f/9.0f);  }

  // Barometric pressure
  inline float hpa_to_inhg(float hpa) { return hpa * 0.02953f; }
  inline float inhg_to_hpa(float inhg) { return inhg / 0.02953f; }

  // Speed
  inline float mph_to_kph(float mph) { return mph * 1.60934f; }
  inline float kph_to_mph(float kph) { return kph / 1.60934f; }
  inline float mps_to_mph(float mps) { return mps* 2.23694; }

  // Length
  inline float in_to_cm(float in) { return in * 2.54f; }
  inline float cm_to_in(float cm) { return cm / 2.54f; }
  inline float in_to_ft(float in) { return in * 12.0f; }
  inline float ft_to_in(float ft) { return ft / 12.0f; }
  inline float m_to_km(float m) { return m * 1.60934f; }
  inline float km_to_m(float km) { return km / 1.60934f; }

  static String EmptyString("");
};
#endif  // Basics_h
