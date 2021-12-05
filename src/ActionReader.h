#ifndef ActionReader_h
#define ActionReader_h

#include <ArduinoJson.h>
#include "ActionManager.h"

namespace ActionReader {
  using ActionFactory = std::function<Action*(String& actionType, JsonObjectConst& settings)>;
	Action* fromJSON(const JsonDocument &doc, ActionFactory factory);
	Action* fromJSON(String filePath, ActionFactory factory);

}
#endif	// ActionReader_h