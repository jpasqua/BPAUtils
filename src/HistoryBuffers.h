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
  using BufferDescriptor = struct {
    HistoryBufferIO* buffer;
    const char* name;
    time_t interval;
  };

  void setBuffer(uint8_t i, const BufferDescriptor& descriptor) {
    if (i >= Size) {
      Log.warning("HistoryBuffers::setBuffer: index out of range %d/%d", i, Size);
      return;
    }
    bufferDescriptors[i] = descriptor;
    descriptor.buffer->setInterval(descriptor.interval);
  }

  bool store(Stream& writeStream) {
    writeStream.print("{ ");
    for (int i = 0; i < Size; i++) {
      if (i) writeStream.print(", ");
      writeStream.print('"'); writeStream.print(bufferDescriptors[i].name); writeStream.print("\":");
      bufferDescriptors[i].buffer->store(writeStream);
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
    for (int i = 0; i < bufferDescriptors.size(); i++) {
      obj = doc[bufferDescriptors[i].name];
      bufferDescriptors[i].buffer->load(obj);
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

  void clear() {
    for (const BufferDescriptor& bd : bufferDescriptors) {
      bd.buffer->clear();
    }
  }

  bool conditionalPushAll(Serializable& item) {
    bool pushed = false;
    for (int i = 0; i < bufferDescriptors.size(); i++) {
      pushed |= bufferDescriptors[i].buffer->conditionalPush(item);
    }
    return pushed;
  }

  void getTimeRange(int index, time_t& start, time_t& end) const {
    start = bufferDescriptors[index].buffer->first().timestamp;
    end = bufferDescriptors[index].buffer->last().timestamp;
  }

  const Serializable& peekAt(size_t bufferIndex, size_t sampleIndex) const {
    return bufferDescriptors[bufferIndex].buffer->peekAt(sampleIndex);
  }

  size_t sizeOfBuffer(size_t bufferIndex) const {
    return bufferDescriptors[bufferIndex].buffer->size();
  }

  const BufferDescriptor& operator[](int index) {
    // Can fail if index is out of bounds
    return bufferDescriptors[index];
  }

private:
  static constexpr size_t MaxHistoryFileSize = 10000;

  std::array<BufferDescriptor, Size> bufferDescriptors;
};

#endif  // HistoryBuffers_h