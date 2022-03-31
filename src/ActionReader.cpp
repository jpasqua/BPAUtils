/*
 * ActionReader.cpp
 *
 */


//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <map>
//                                  Third Party Libraries
#include <ESP_FS.h>
#include <GenericESP.h>
#include <ArduinoLog.h>
#include <ArduinoJson.h>
//                                  Personal Libraries
//                                  App Libraries and Includes
#include "ActionReader.h"
//--------------- End:    Includes ---------------------------------------------


namespace ActionReader {
	namespace internal {
		constexpr size_t MaxDocSize = 8192;
		std::map<String,Action*> actionMap;


		PauseAction* internalizePause(JsonObjectConst& settings) {
			uint32_t pause = settings["pause"];
			return new PauseAction(pause);
		}

		RepeatAction* internalizeRepeat(JsonObjectConst& settings) {
			uint32_t nTimes = settings["nTimes"];
			uint32_t pause = settings["pause"];
			auto mapping = actionMap.find(settings["actionID"].as<String>());
			if (mapping == actionMap.end()) return nullptr;
			return new RepeatAction(mapping->second, nTimes, pause);
		}

		SequenceAction* internalizeSequence(JsonObjectConst& settings) {
			Actions targetActions;

			uint32_t pause = settings["pause"];
	  	JsonArrayConst json_actionNames = settings[F("actions")];
		  for (const auto& json_actionName : json_actionNames) {
		  	String name = json_actionName.as<String>();
				auto mapping = actionMap.find(name);
				if (mapping != actionMap.end()) {
					targetActions.push_back(mapping->second);
				}
		  }
		  if (targetActions.size() == 0) {
		  	Log.warning("Sequenece with no actions");
		  	return nullptr;
		  }
		  return new SequenceAction(targetActions, pause);
		}


		Action* uberFactory(String type, JsonObjectConst& settings, ActionFactory factory) {
			Action* a = nullptr;

			if (type.equalsIgnoreCase("Pause")) a = internalizePause(settings);
			else if (type.equalsIgnoreCase("Repeat")) a = internalizeRepeat(settings);
			else if (type.equalsIgnoreCase("Sequence")) a = internalizeSequence(settings);
      else a = factory(type, settings);

			return a;
		}

		char *makeAHole() {
	    // The following call to malloc is actually there to *reduce* fragmentation! What happens normally
	    // is we allocate a huge chunk for the the JSON document, read it in, and then allocate the various
	    // FlexScreen objects. They are allocated "later" on the heap than the JSON document, so when we free
	    // it, there is a big hole left over (hence the fragmentation). By allocating the placeholder first,
	    // then allocating the JSON doc, then freeing the placeholder, we leave space "before" the JSON doc
	    // for the FlexScreen items to consume. Then when the JSON doc is freed, it doesnt leave a hole since
	    // it is freed from the end of the heap.
	    constexpr uint16_t PluginReserveSize = 2000;
	    constexpr uint16_t PlaceHolderSize = 2000;
	    char *placeHolder = nullptr;

	    if (GenericESP::getMaxFreeBlockSize() > PlaceHolderSize + PluginReserveSize) {
	      placeHolder = (char*)malloc(PluginReserveSize);
	      placeHolder[1] = 'C';   // Touch the memory or the compiler may optimize away the malloc
	    }
	    return placeHolder;
	  }

	}	// END: ActionReader::Internal namespace


  SequenceAction* fromJSON(const JsonDocument &doc, ActionFactory factory) {
	  JsonArrayConst json_actions = doc[F("actions")];

	  for (JsonObjectConst json_action : json_actions) {
	  	String type = json_action["type"].as<String>();
	  	String id = json_action["id"].as<String>();
	  	JsonObjectConst settings = json_action["settings"];

      Action* a = internal::uberFactory(type, settings, factory);

	  	if (a == nullptr) {
	  		Log.warning("Unknown Action type: %s", type.c_str());
	  	} else if (id == "main" && !type.equalsIgnoreCase("Sequence")) {
  			Log.warning("Main action must be a Sequence, but is %s", type.c_str());
  		} else {
	  		internal::actionMap[id] = a;
  		}
	  }

	  auto mapping = internal::actionMap.find("main");
		return (SequenceAction*)((mapping == internal::actionMap.end()) ? nullptr : mapping->second);
	}

  SequenceAction* fromJSON(String filePath, ActionFactory factory) {
	  File actionFile = ESP_FS::open(filePath, "r");
	  if (!actionFile) {
	  	Log.warning("No action file was found: %s", filePath.c_str());
	    return nullptr;
	  }

	  char* hole = internal::makeAHole();
	  DynamicJsonDocument doc(internal::MaxDocSize);
	  if (hole) free(hole);

	  auto error = deserializeJson(doc, actionFile);
	  if (error) {
	    Log.warning(F("Error parsing actions: %s"), error.c_str());
	    return nullptr;
	  }

	  Log.verbose("Successfully read action file: %s", filePath.c_str());
	  return fromJSON(doc, factory);
	}
}