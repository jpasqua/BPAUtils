#ifndef GenericESP_h
#define GenericESP_h

#include <Arduino.h>

namespace GenericESP {

  // ----- System
  uint32_t getChipID();
  void reset();

  // ----- Heap Stats
  uint32_t getFreeHeap();
  uint8_t getHeapFragmentation();
  uint16_t getMaxFreeBlockSize();

#if defined(ESP8266)
  constexpr uint16_t getADCMax() { return 1023; }
#elif defined(ESP32)
  constexpr uint16_t getADCMax() { return 4095; }
#else
  #error "Must be an ESP8266 or ESP32"
#endif  
}

#endif // GenericESP_h