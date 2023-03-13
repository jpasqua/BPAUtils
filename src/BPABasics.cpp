#include "BPABasics.h"

namespace Basics {
  // ----- String Related
  char* newFromString(String& source) {
    uint32_t destLen = source.length()+1;
    char* dest = (char*)malloc(destLen);
    source.toCharArray(dest, destLen);
    return dest;
  }
}
