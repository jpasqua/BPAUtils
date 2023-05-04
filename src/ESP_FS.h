#ifndef ESP_FS_h
#define ESP_FS_h

#include <FS.h>

namespace ESP_FS {
  FS* getFS();
  bool begin();
  bool format();

  File open(const char* path, const char* mode);
  File open(const String& path, const char* mode);

  bool exists(const char* path);
  bool exists(const String& path);

  bool remove(const String& path);
  bool rename(const char* pathFrom, const char* pathTo);
  bool move(const char* pathFrom, const char* pathTo);
  
  class DirEnumerator {
  public:
    virtual ~DirEnumerator() {};  // Must provide body for pure virtual destructor!
    virtual bool begin(const String& path) = 0;
    virtual bool next(String& name) = 0;  // NOTE: name is an "out" parameter
  };

  DirEnumerator* newEnumerator();
};

// To enumerate the entire filesystem, one could use:
// void enumFS() {
//   ESP_FS::DirEnumerator* de = ESP_FS::newEnumerator();
//   de->begin("/");
//   String path;
//   while (de->next(path)) { Log.verbose("File: %s", path.c_str());  }
//   delete de;
// }


#endif  // ESP_FS_h