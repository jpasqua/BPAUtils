#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <ESP_FS.h>
#include <Serializable.h>
#include <HistoryBuffer.h>
#include <HistoryBuffers.h>
#include "THPReadings.h"

void flushSerial(Print *p) { p->print(CR); Serial.flush(); }

void prepLogging() {
  Serial.begin(115200); while (!Serial) delay(20);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial, false);
  Log.setSuffix(flushSerial);

  // Separate out from the normal garbage that starts the output
  Serial.println(); Serial.flush(); delay(100);
  Serial.println(); Serial.flush(); delay(100);
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

void genRandomData(HistoryBuffer<THPReadings, 5>& historyBuffer) {
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
  HistoryBuffer<THPReadings, 5> historyBuffer;
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
  }
}

void testPushingRandomData() {
  HistoryBuffer<THPReadings, 5> historyBuffer;
  genRandomData(historyBuffer);

  Log.verbose("About to serialize to the console");
  HistoryBufferIO* io = &historyBuffer;
  io->store(Serial);
}

void testLoadAndStoreToFile() {
  HistoryBuffer<THPReadings, 5> historyBuffer;
  genRandomData(historyBuffer);

  Log.verbose("About to store to file");
  String historyFilePath = "/temp/history.json";
  historyBuffer.store(historyFilePath);

  historyBuffer.clear();
  historyBuffer.load(historyFilePath);

  Log.verbose("About to serialize to the console");
  historyBuffer.store(Serial);  
}

void testHistoryBuffers() {
  HistoryBuffers<3> buffers;
  HistoryBuffer<THPReadings, 5> hour;
  HistoryBuffer<THPReadings, 5> day;
  HistoryBuffer<THPReadings, 5> week;

  genRandomData(hour);
  genRandomData(day);
  genRandomData(week);

  buffers.setBuffer(&day, "day", 0);
  buffers.setBuffer(&hour, "hour", 1);
  buffers.setBuffer(&week, "week", 2);

  Log.verbose("About store a HistoryBuffers object");
  buffers.store("/buffers.json");

  Log.verbose("Clearing the individual buffers");
  hour.clear();
  day.clear();
  week.clear();

  Log.verbose("Reloading the buffers from a file");
  buffers.load("/buffers.json");

  Log.verbose("Displaying loaded values");
  hour.store(Serial);
  day.store(Serial);
  week.store(Serial);
}

void setup() {
	prepLogging();
  prepFS();

  testPushingRandomData();
  testLoadAndStoreToFile();
  testLoadingFromObj();

  testHistoryBuffers();
}

void loop() {

}