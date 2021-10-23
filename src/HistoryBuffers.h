/*
 * HistoryBuffers
 *     Manage a collection of related HistoryBuffer objects
 *
 * CONSIDER:
 * o It can be very memory intensive to load from a single file
 *   that contains multiple HistoryBuffer serializations. To get
 *   around this we could either:
 *   a) Save each history buffer to a separate file and load from
 *      those files OR
 *   b) Use a filter document and load the same file N times using each
 *      buffer's name to get just the data in question.
 *   The former should, in theory, be faster, but from a developer
 *   perspective it's kind of a pain to have multiple files if you want
 *   to read back and preserve state.
 * o Think about adding a max size to the buffer descriptor to guide
 *   how much space to allocate.
 *
 */

#ifndef HistoryBuffers_h
#define HistoryBuffers_h

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
#include <array>
//                                  Third Party Libraries
#include <ArduinoLog.h>
#include <ArduinoJson.h>
//                                  WebThing Includes
#include <ESP_FS.h>
//                                  Local Includes
#include "HistoryBuffer.h"
//--------------- End:    Includes ---------------------------------------------


template<typename BufferType, int Size>
class HistoryBuffers {
public:

/*------------------------------------------------------------------------------
 *
 * Construct / Destruct / Initialize
 *
 *----------------------------------------------------------------------------*/

  HistoryBuffers() = default;

  void describe(const HBDescriptor& descriptor) {
    buffers[nBuffersDescribed++].init(descriptor);
  }

/*------------------------------------------------------------------------------
 *
 * Internalize / Externalize HistoryBuffers
 *
 *----------------------------------------------------------------------------*/

  bool store(Stream& writeStream) {
    writeStream.print("{ ");
    for (int i = 0; i < Size; i++) {
      if (i) writeStream.print(", ");
      writeStream.print('"'); writeStream.print(buffers[i]._name); writeStream.print("\":");
      buffers[i].store(writeStream);
    }
    writeStream.print(" }");

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

    if (success) Log.verbose("HistoryBuffers written to file: %s", historyFilePath.c_str());
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
    for (int i = 0; i < Size; i++) {
      obj = doc[buffers[i]._name];
      buffers[i].load(obj);
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

/*------------------------------------------------------------------------------
 *
 * Access / Inspect / Modify elements of the HistoryBuffer
 *
 *----------------------------------------------------------------------------*/

  void clearAll() {
    for (int i = 0; i < Size; i++) {
      buffers[i].clear();
    }
  }

  bool conditionalPushAll(BufferType& item) {
    bool pushed = false;
    for (int i = 0; i < Size; i++) {
      pushed |= buffers[i].conditionalPush(item);
    }
    return pushed;
  }

  const HistoryBuffer<BufferType>& operator[](int index) const {
    return buffers[index];
  }

  HistoryBuffer<BufferType>& getMutable(int index) {
    return buffers[index];
  }


private:
  static constexpr size_t MaxHistoryFileSize = 10000;

  uint8_t nBuffersDescribed = 0;
  HistoryBuffer<BufferType> buffers[Size];
};

#endif  // HistoryBuffers_h