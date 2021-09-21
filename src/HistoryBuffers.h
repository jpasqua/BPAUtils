/*
 * HistoryBuffers
 *     WRITE ME
 *
 * NOTES:
 * o WRITE ME
 *
 */

#ifndef HistoryBuffers_h
#define HistoryBuffers_h

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
#include <ArduinoLog.h>
#include <ArduinoJson.h>
//                                  WebThing Includes
#include <ESP_FS.h>
//                                  Local Includes
#include "HistoryBuffer.h"
//--------------- End:    Includes ---------------------------------------------


template<int Size>
class HistoryBuffers {
public:
  void setBuffer(HistoryBufferIO* buffer, const char* name, uint8_t i) {
    if (i >= Size) {
      Log.warning("HistoryBuffers::setBuffer: index out of range %d/%d", i, Size);
      return;
    }
    names[i] = name;
    buffers[i] = buffer;
  }

  bool store(Stream& writeStream) {
    writeStream.print("{ ");

    for (int i = 0; i < Size; i++) {
      if (i) writeStream.print(",\n");
      writeStream.print('"'); writeStream.print(names[i]); writeStream.print("\":");
      buffers[i]->store(writeStream);
    }

    writeStream.print(" }");

    Log.verbose("HistoryBuffers written to stream");
    return true;
  }

  bool store(const String& historyFilePath) {
    File historyFile = ESP_FS::open(historyFilePath, "w");

    if (!historyFile) {
      Log.error(F("Failed to open history file for writing: %s"), historyFilePath.c_str());
      return false;
    }

    bool success = store(historyFile);
    historyFile.close();

    if (success) Log.verbose("HistoryBuffers written written to file");
    else Log.warning("Error saving history to %s", historyFilePath.c_str());
    
    return success;
  }

  bool load(Stream& readStream) {
    DynamicJsonDocument doc(MaxHistoryFileSize);

    auto error = deserializeJson(doc, readStream);
    if (error) {
      Log.warning(F("Failed to parse history stream: %s"), error.c_str());
      return false;
    }

    JsonObjectConst obj;
    for (int i = 0; i < buffers.size(); i++) {
      obj = doc[names[i]];
      buffers[i]->load(obj);
    }

    return true;
  }

  bool load(const String& historyFilePath) {
    size_t size = 0;
    File historyFile = ESP_FS::open(historyFilePath, "r");

    if (!historyFile) {
      Log.error(F("Failed to open history file for read: %s"), historyFilePath.c_str());
      return false;
    } else {
      size = historyFile.size();
      if (size > MaxHistoryFileSize) {
        Log.warning(F("HistoryBuffer file is too big: %d"), size);
        historyFile.close();
        return false;
      }    
    }

    bool success = load(historyFile);
    historyFile.close();

    if (success) Log.verbose("HistoryBuffers loaded from %s", historyFilePath.c_str());
    else Log.warning("Error loading history from %s", historyFilePath.c_str());
    
    return success;
  }

  bool conditionalPushAll(Serializable& item) {
    bool pushed = false;
    for (int i = 0; i < buffers.size(); i++) {
      pushed |= buffers[i]->conditionalPush(item);
    }
    return pushed;
  }

private:
  static constexpr size_t MaxHistoryFileSize = 8192;

  std::array<HistoryBufferIO*, Size> buffers;
  std::array<String, Size> names;  

};

#endif  // HistoryBuffers_h