/*
 * ESP_FS
 *    Implementation of the ESP_FS interface
 *
 */

#define ESPFS_DEBUG_LOG

#if defined(ESPFS_DEBUG_LOG)
  #include<ArduinoLog.h>
#endif

#if defined(ESP32)
  #include <SPIFFS.h>
#endif
#include "ESP_FS.h"

// Common items
  namespace ESP_FS {
    // SPIFFS is being deprecated on ESP8266 which causes warnings at compile time.
    // I have a task to move off of SPIFFS to LittleFS, but in the mean time, I
    // don't want to keep seeing the warnings so I wrapped the SPIFFS calls with
    // pragma's to silence the warnings

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    FS* getFS() { return &SPIFFS; }
    bool begin() { return SPIFFS.begin(); }
    bool format() { return SPIFFS.format(); }

    File open(const char* path, const char* mode) { return SPIFFS.open(path, mode); }
    File open(const String& path, const char* mode) { return SPIFFS.open(path, mode); }

    bool exists(const char* path) { return SPIFFS.exists(path); }
    bool exists(const String& path) { return SPIFFS.exists(path); }

    bool remove(const String& path) {
      return SPIFFS.remove(path);
    }

    bool rename(const char* pathFrom, const char* pathTo) {
      return SPIFFS.rename(pathFrom, pathTo);
    }

    bool move(const char* pathFrom, const char* pathTo) {
      File src = open(pathFrom, "r");
      if (!src) {
        Log.warning("ESP_FS::move: Failed to open source: %s", pathFrom);
        return false;
      }
      File dest = open(pathTo, "w");
      if (!dest) {
        Log.warning("ESP_FS::move: Failed to open dest: %s", pathTo);
        return false;
      }
      const size_t BufSize = 128;
      byte buf[BufSize];
      int bytesRead;
      while ((bytesRead = src.read(buf, BufSize))) {
        // Log.verbose("Read %d bytes from source", bytesRead);
        int bytesWritten = dest.write(buf, bytesRead);
        if (bytesWritten != bytesRead) {
          Log.warning("Only wrote %d bytes, expected %d bytes", bytesWritten, bytesRead);
        }
      }
      dest.close();
      src.close();
      return remove(pathFrom);
    }
    #pragma GCC diagnostic pop
  };


#if defined(ESP32)

  namespace ESP_FS {
    class ESP32DirEnumerator : public DirEnumerator {
    public:
      ~ESP32DirEnumerator() { }
      bool begin(const String& path) override {
        enumRoot = SPIFFS.open(path);
        return (enumRoot.isDirectory());
      }
      bool next(String& name) override {
        File f;
        do { f = enumRoot.openNextFile(); } while (f && f.isDirectory());
        if (!f) return false;
        name = f.path();

        return true;
      }
    private:
      File enumRoot;
    };

    DirEnumerator* newEnumerator() { return new ESP32DirEnumerator(); }
  };

#elif defined(ESP8266)
  namespace ESP_FS {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
      Dir openDir(const char* path) { return SPIFFS.openDir(path); }
      Dir openDir(const String& path) { return SPIFFS.openDir(path); }
    #pragma GCC diagnostic pop

    class ESP8266DirEnumerator : public DirEnumerator {
    public:
      ~ESP8266DirEnumerator() { }
      bool begin(const String& path) override {
        enumRoot = ESP_FS::openDir(path);
        return true;  // Always returns true, even if it is empty
      }
      bool next(String& name) override {
        if (!enumRoot.next()) return false;
        name = enumRoot.fileName();
        return true;
      }
    private:
      Dir enumRoot;
    };

    DirEnumerator* newEnumerator() { return new ESP8266DirEnumerator(); }
  };

#endif  
