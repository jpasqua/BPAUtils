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
//                                  WebThing Includes
#include <ESP_FS.h>
//                                  Local Includes
#include "BPACircularBuffer.h"
#include "Serializable.h"
//--------------- End:    Includes ---------------------------------------------


using HBDescriptor = struct {
  size_t nElements;
  const char* name;
  time_t interval;
};

class HistoryBufferBase {
public:
  time_t _interval = 0;
  const char* _name;

  // ----- Abstract Member Functions

  virtual size_t size() const = 0;
  virtual const Serializable& first() const = 0;
  virtual const Serializable& last() const = 0;
  virtual const Serializable& peekAt(size_t index) const = 0;

  virtual void clear() = 0;
  virtual void push(JsonObjectConst jsonItem) = 0;

  // NOTE: Caller must guarantee that the Serializable that they provide
  // is in fact of the type required by the concrete derived class.
  // There is no checking and no dynamic_cast!
  virtual bool push(const Serializable& item) = 0;
  virtual bool conditionalPush(const Serializable& item) = 0;


  // ----- Concrete Member Functions

  bool store(Stream& writeStream) const {
    writePreamble(writeStream);

    // Write the items
    for (size_t i = 0; i < size(); i++) {
      if (i) writeStream.print(',');
      const Serializable& item = peekAt(i);
      item.externalize(writeStream);
    }

    writePostscript(writeStream);

    return true;
  }

  bool store(const String& historyFilePath) const {
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

  bool load(const JsonObjectConst& historyElement) {
    clear();  // Start from scratch...

    JsonArrayConst historyArray = historyElement[F("history")];

    uint32_t nLoaded = 0;
    for (JsonObjectConst jsonItem : historyArray) {
      push(jsonItem);
      nLoaded++;
    }
    if (nLoaded) {
      _lastTimeStamp = last().timestamp;
    }

    return true;
  }

  bool load(Stream& readStream) {
    DynamicJsonDocument doc(MaxHistoryFileSize);
    auto error = deserializeJson(doc, readStream);
    if (error) {
      Log.warning(F("HistoryBuffer::load: Parse error: %s"), error.c_str());
      return false;
    }

    const JsonObjectConst root = doc.as<JsonObjectConst>();
    return load(root);
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

    if (success) Log.verbose("HistoryBuffer data loaded");
    else Log.warning("Error loading history from %s", historyFilePath.c_str());
    
    return success;
  }

  void getTimeRange(time_t& start, time_t& end) const {
    start = first().timestamp;
    end = last().timestamp;
  }

protected:
  void writePreamble(Stream& writeStream) const {
    writeStream.print("{ \"history\": [");
  }

  void writePostscript(Stream& writeStream) const {
    writeStream.println("]}");
    writeStream.flush();
  }

  time_t _lastTimeStamp = 0;

private:
  static constexpr size_t MaxHistoryFileSize = 12000;

};


template<typename ItemType>
class HistoryBuffer  : public HistoryBufferBase {

public:

/*------------------------------------------------------------------------------
 *
 * Construct / Destruct / Initialize
 *
 *----------------------------------------------------------------------------*/

  HistoryBuffer() = default;

  HistoryBuffer(const HBDescriptor& desc, ItemType* space = nullptr) {
    init(desc, space);
  }

  void init(const HBDescriptor& desc, ItemType* space = nullptr) {
    if (space) _historyItems.init(space, desc.nElements);
    else _historyItems.init(desc.nElements);
    _name = desc.name;
    _interval = desc.interval;
  }


/*------------------------------------------------------------------------------
 *
 * Member functions that are introduced in this derived class (not in base)
 *
 *----------------------------------------------------------------------------*/

  inline bool conditionalPush(const ItemType& item) {
    if (item.timestamp - _lastTimeStamp >= _interval) {
      _historyItems.push(item);
      _lastTimeStamp = item.timestamp;
      return true;
    }
    return false;
  }

  inline bool push(const ItemType& item) { return _historyItems.push(item);  }


/*------------------------------------------------------------------------------
 *
 * Implementation of the HistoryBufferBase interface
 *
 *----------------------------------------------------------------------------*/

  virtual size_t size() const override { return _historyItems.size(); } 
  virtual const ItemType& first() const override { return _historyItems.peekAt(0);  }
  virtual const ItemType& last() const override { return _historyItems.peekAt(_historyItems.size()-1); }
  virtual const ItemType& peekAt(size_t index) const override { return _historyItems.peekAt(index); }

  virtual void clear() override { _historyItems.clear(); }

  virtual void push(JsonObjectConst jsonItem) override {
    ItemType item;
    item.internalize(jsonItem);
    _historyItems.push(item);
  }

  virtual bool push(const Serializable& item) override {
    // We know based on the assert below that ItemType isa Serializable,
    // but it is up to the caller to ensure that this particulat Serializable
    // isa ItemType
    return _historyItems.push(static_cast<const ItemType&>(item)); 
  }

  virtual bool conditionalPush(const Serializable& item) override {
    // We know based on the assert below that ItemType isa Serializable,
    // but it is up to the caller to ensure that this particulat Serializable
    // isa ItemType
    return conditionalPush(static_cast<const ItemType&>(item));
  }

private:
  static_assert(std::is_base_of<Serializable, ItemType>::value, "HistoryBuffer Item must derive from Serializable");
	BPACircularBuffer<ItemType> _historyItems;
  
};

#endif  // HistoryBuffer_h