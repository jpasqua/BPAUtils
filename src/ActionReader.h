#ifndef ActionReader_h
#define ActionReader_h

#include <functional>
#include <ArduinoJson.h>
#include "ActionManager.h"

namespace ActionReader {
  using ActionFactory = std::function<Action*(String& actionType, JsonObjectConst& settings)>;
	SequenceAction* fromJSON(const JsonDocument &doc, ActionFactory factory);
	SequenceAction* fromJSON(String filePath, ActionFactory factory);
}

#endif	// ActionReader_h