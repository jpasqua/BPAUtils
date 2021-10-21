/*
 * HistoryBuffer
 *     Keeps track of an ordered series of data and provides functionality
 *     to externalize as JSON and internalize from JSON
 *
 * NOTES:
 * o The type you specify as a template parameter to HistoryBuffer must
 *   implement internalize() and externalize() methods.
 * o When using the HistoryBuffer template class, you specify (statically) the
 *   max number of elements and their type. These elements are allocated as
 *   part of the HistoryBuffer object (not separately in the heap).
 * o You can add elements to the history up to the size limit, and once
 *   that limit is reached, adding a new hisotry element will cause the
 *   oldest to be removed.
 * o Timestamps: If your data includes timestamps, they should be in 
 *   "wall clock" time rather than millis() since that gets reset on
 *   every boot. It is usually best to use times in GMT
 *
 */

#ifndef HistoryBuffer_h
#define HistoryBuffer_h

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
#include <ArduinoLog.h>
#include <ArduinoJson.h>
#include <CircularBuffer.h>
//                                  WebThing Includes
#include <ESP_FS.h>
//                                  Local Includes
#include "Serializable.h"
//--------------- End:    Includes ---------------------------------------------


class HistoryBufferIO {
public:
  virtual bool store(Stream& writeStream) = 0;
  virtual bool store(const String& historyFilePath) = 0;
  virtual bool load(Stream& readStream) = 0;
  virtual bool load(const JsonObjectConst& historyElement) = 0;
  virtual bool load(const String& historyFilePath) = 0;

  virtual bool conditionalPush(Serializable& item) = 0;
  virtual void clear() = 0;
  virtual const Serializable& peekAt(size_t index) const = 0;
  virtual const Serializable& first() const = 0;
  virtual const Serializable& last() const = 0;

  virtual size_t size() const;

  void setInterval(time_t interval) { _interval = interval; }

protected:
  time_t _interval = 0;
};

template<typename ItemType, int Size>
class HistoryBuffer : public HistoryBufferIO {
private:
	static constexpr size_t MaxHistoryFileSize = 12000;
  static_assert(std::is_base_of<Serializable, ItemType>::value, "HistoryBuffer Item must derive from Serializable");
  

public:
  //
  // Member functions overriden from HistoryBufferIO
  //

  bool store(Stream& writeStream) override {
    writePreamble(writeStream);

    // Write the items
    for (size_t i = 0; i < _historyItems.size(); i++) {
      if (i) writeStream.print(',');
      const ItemType& item = _historyItems.peekAt(i);
      item.externalize(writeStream);
    }

    writePostscript(writeStream);

    return true;
  }

  bool store(const String& historyFilePath) override {
    File historyFile = ESP_FS::open(historyFilePath, "w");

    if (!historyFile) {
      Log.error(F("Failed to open history file for writing: %s"), historyFilePath.c_str());
      return false;
    }

    bool success = store(historyFile);
    historyFile.close();

    if (success) Log.verbose("HistoryBuffer written written to file: %s", historyFilePath.c_str());
    else Log.warning("Error saving history to %s", historyFilePath.c_str());
    return success;
  }

  bool load(const JsonObjectConst& historyElement) override {
    _historyItems.clear();  // Start from scratch...

    JsonArrayConst historyArray = historyElement[F("history")];

    ItemType item;
    uint32_t nLoaded = 0;
    for (JsonObjectConst jsonItem : historyArray) {
      item.internalize(jsonItem);
      _historyItems.push(item);
      nLoaded++;
    }
    if (nLoaded) {
      const ItemType& last =  _historyItems.peekAt(nLoaded-1);
      _lastTimeStamp = last.timestamp;
    }

    return true;
  }

  bool load(Stream& readStream) override {

    DynamicJsonDocument doc(MaxHistoryFileSize);
    auto error = deserializeJson(doc, readStream);
    if (error) {
      Log.warning(F("HistoryBuffer::load: Parse error: %s"), error.c_str());
      return false;
    }

    const JsonObjectConst root = doc.as<JsonObjectConst>();
    return load(root);
  }

  bool load(const String& historyFilePath) override {
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

    if (success) Log.verbose("HistoryBuffer data loaded");
    else Log.warning("Error loading history from %s", historyFilePath.c_str());
    
    return success;
  }

  virtual bool conditionalPush(Serializable& item) override {
    // TO DO: This is not safe. We should do something to ensure that
    // the provided Serializable item is in fact of type ItemType
    // such as a dynamic_cast or perhaps some sort of CRTP structure
    return conditionalPush(static_cast<ItemType&>(item));
  }

  //
  // Member functions directly from HistoryBuffer
  //

  inline bool conditionalPush(const ItemType& item) {
    if (item.timestamp - _lastTimeStamp >= _interval) {
      _historyItems.push(item);
      _lastTimeStamp = item.timestamp;
      return true;
    }
    return false;
  }

  inline bool push(const ItemType& item) {
    return _historyItems.push(item);
  }

  inline const ItemType& peekAt(size_t index) const {
    return _historyItems.peekAt(index);
  }

  inline const ItemType& first() const {
    return _historyItems.peekAt(0);
  }

  inline const ItemType& last() const {
    return _historyItems.peekAt(_historyItems.size()-1);
  }

  inline void clear() { _historyItems.clear(); }

  constexpr size_t size() const override { return _historyItems.size(); } 

private:
	CircularBuffer<ItemType, Size> _historyItems;
  time_t _lastTimeStamp = 0;
  
  void writePreamble(Stream& writeStream) {
    writeStream.print("{ \"history\": [");
  }

  void writePostscript(Stream& writeStream) {
    writeStream.println("]}");
    writeStream.flush();
  }
};

#endif  // HistoryBuffer_h