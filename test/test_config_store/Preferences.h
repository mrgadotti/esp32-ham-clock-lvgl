#pragma once
// Native stand-in for Arduino-ESP32 Preferences: one shared in-memory NVS, with a hook run on every put.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>

inline std::map<std::string, std::string> g_stubNvs;
inline int g_stubPuts = 0;
inline bool g_stubFailPuts = false;
inline void (*g_stubOnPut)() = nullptr;

class Preferences {
 public:
  bool begin(const char*, bool) { return true; }
  void end() {}
  bool isKey(const char* k) { return g_stubNvs.count(k) != 0; }
  size_t getString(const char* k, char* d, size_t n) { snprintf(d, n, "%s", g_stubNvs[k].c_str()); return strlen(d); }
  int32_t getInt(const char* k) { return get<int32_t>(k); }
  float getFloat(const char* k) { return get<float>(k); }
  bool getBool(const char* k) { return get<uint8_t>(k) != 0; }
  uint8_t getUChar(const char* k) { return get<uint8_t>(k); }
  size_t getBytesLength(const char* k) { return g_stubNvs[k].size(); }
  size_t getBytes(const char* k, void* d, size_t) { memcpy(d, g_stubNvs[k].data(), g_stubNvs[k].size()); return g_stubNvs[k].size(); }
  size_t putString(const char* k, const char* v) { return put(k, v, strlen(v)); }
  size_t putInt(const char* k, int32_t v) { return put(k, &v, 4); }
  size_t putFloat(const char* k, float v) { return put(k, &v, 4); }
  size_t putBool(const char* k, bool v) { uint8_t b = v; return put(k, &b, 1); }
  size_t putUChar(const char* k, uint8_t v) { return put(k, &v, 1); }
  size_t putBytes(const char* k, const void* v, size_t n) { return put(k, v, n); }

 private:
  template <class T> T get(const char* k) { T v{}; memcpy(&v, g_stubNvs[k].data(), sizeof v); return v; }
  size_t put(const char* k, const void* v, size_t n) {
    g_stubPuts++;
    if (g_stubOnPut) g_stubOnPut();
    if (g_stubFailPuts) return 0;
    g_stubNvs[k].assign((const char*)v, n);
    return n;
  }
};
