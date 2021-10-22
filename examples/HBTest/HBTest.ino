#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <ESP_FS.h>
#include <Serializable.h>
#include <HistoryBuffer.h>
#include <HistoryBuffers.h>
#include "THPReadings.h"
#include "BPABasics.h"

void flushSerial(Print *p) { p->print(CR); Serial.flush(); }

void prepLogging() {
  Serial.begin(115200); while (!Serial) delay(20);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial, false);
  Log.setSuffix(flushSerial);

  // Separate out from the normal garbage that starts the output
  delay(200);
  Serial.print("\n\n");
}

void prepFS() {
  boolean mounted = ESP_FS::begin();
  if (!mounted) {
    Log.notice(F("FS not formatted. Formatting now. This can take >= 30 seconds."));
    ESP_FS::format();
    ESP_FS::begin();
    Log.trace(F("Completed FS Formatting"));
  }
}

const char* TestData = 
"{ \"element\": {"
"    \"tzOffset\": 4294938496,"
"    \"history\":"
"    ["
"        {"
"            \"temp\": 48,"
"            \"humidity\": 62.1,"
"            \"pressure\": 671.5"
"        },"
"        {"
"            \"temp\": 40.7,"
"            \"humidity\": 24.7,"
"            \"pressure\": 942.9"
"        },"
"        {"
"            \"temp\": 85.6,"
"            \"humidity\": 37.6,"
"            \"pressure\": 774.6"
"        },"
"        {"
"            \"temp\": 27,"
"            \"humidity\": 83.2,"
"            \"pressure\": 182.1"
"        },"
"        {"
"            \"temp\": 26.7,"
"            \"humidity\": 77,"
"            \"pressure\": 723"
"        }"
"    ]"
"} }";

void genRandomData(HistoryBuffer<THPReadings>& historyBuffer) {
  THPReadings item;
  for (int i = 1; i < 10; i++) {
    item.temp = ((float)random(1000))/10;
    item.humidity = ((float)random(1000))/10;
    item.pressure = ((float)random(10000))/10;
    item.calculateDerivedValues();
    historyBuffer.push(item);
  }
}

void testLoadingFromObj() {
  HistoryBuffer<THPReadings> historyBuffer({5, "test", 23});
  StaticJsonDocument<1024> doc;
  auto error = deserializeJson(doc, TestData);
  if (error) {
    Log.warning(F("Failed to TestData"), error.c_str());
  } else {
    Log.verbose("Loading from a JSONObject");
    JsonObjectConst element = doc["element"];
    historyBuffer.clear();
    historyBuffer.load(element);
    Log.verbose("About to serialize to the console");
    historyBuffer.store(Serial);
    Log.verbose("\n===== Test: Complete");
  }
}

void testPushingRandomData() {
  Log.verbose("\n===== Test: Push randomly generated items into a buffer and print it");
  HistoryBuffer<THPReadings> historyBuffer({5, "test", 23});
  genRandomData(historyBuffer);

  Log.verbose("-- About to serialize to the console");
  historyBuffer.store(Serial);
  Log.verbose("\n===== Test: Complete");
}

void testLoadAndStoreToFile() {
  Log.verbose("\n===== Test: Store a single buffer of randomly generated data to a file");
  HistoryBuffer<THPReadings> historyBuffer({5, "test", 23});  // Use constructor based initialization
  genRandomData(historyBuffer);

  String historyFilePath = "/temp/history.json";
  historyBuffer.store(historyFilePath);

  historyBuffer.clear();
  historyBuffer.load(historyFilePath);

  Log.verbose("-- About to serialize to the console");
  historyBuffer.store(Serial);  
  Log.verbose("\n===== Test: Complete");
}

void testHistoryBuffers() {
  Log.verbose("\n===== Test: Storing multiple buffers using a HistoryBuffers object (v1)");
  HistoryBuffers<THPReadings, 3> buffers;
  buffers.describe({12, "hour", minutesToTime_t(5)});
  buffers.describe({24, "day", hoursToTime_t(1)});
  buffers.describe({28, "week", hoursToTime_t(6)});

  genRandomData(buffers.getMutable(0));
  genRandomData(buffers.getMutable(1));
  genRandomData(buffers.getMutable(2));

  Log.verbose("\n-- About store a HistoryBuffers object");
  buffers.store("/buffers.json");

  Log.verbose("\n-- Clearing the individual buffers");
  buffers.clearAll();

  Log.verbose("\n-- Reloading the buffers from a file");
  buffers.load("/buffers.json");

  Log.verbose("\n-- Display the loaded values");
  buffers.store(Serial);
  Log.verbose("\n===== Test: Complete");}

void testHistoryBuffers2() {
  // Log.verbose("\n===== Test: Storing multiple buffers using a HistoryBuffers object (v2)");
  // HistoryBuffers<THPReadings, 3> buffers;

  // HBDescriptor descriptors[] {
  //   {4, "hour", 23},
  //   {5, "day", 34},
  //   {6, "week", 119}
  // };
  // THPReadings spaceForAllHistories[15];

  // buffers.init(descriptors, spaceForAllHistories);

  // genRandomData(buffers[0]);
  // genRandomData(buffers[1]);
  // genRandomData(buffers[2]);

  // // buffers.setBuffer(0, {&rawBuffers[0], "1", 12});
  // // buffers.setBuffer(1, {&rawBuffers[1], "2", 24});
  // // buffers.setBuffer(2, {&rawBuffers[2], "3", 28});

  // Log.verbose("\n-- About store a HistoryBuffers object");
  // buffers.store("/buffers.json");

  // Log.verbose("\n-- Clearing the individual buffers");
  // buffers.clear();

  // Log.verbose("\n-- Reloading the buffers from a file");
  // buffers.load("/buffers.json");

  // Log.verbose("\n-- Display the loaded values");
  // buffers.store(Serial);
  // Log.verbose("\n===== Test: Complete");
}


void setup() {
	prepLogging();
  prepFS();

  testPushingRandomData();
  testLoadAndStoreToFile();
  testLoadingFromObj();

  testHistoryBuffers();
  testHistoryBuffers2();
}

void loop() {

}