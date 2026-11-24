#pragma once
#include <Arduino.h>
#include <atomic>
#include <freertos/semphr.h>

#include "ring_buffer.h"
#include "types.h"

// Mutex-guarded value. Callbacks run under the lock: keep them short and never nest stores.
template <typename T>
class Guarded {
 public:
  Guarded() : mu_(xSemaphoreCreateMutex()) { configASSERT(mu_ != nullptr); }
  template <class F> void write(F f) {
    xSemaphoreTake(mu_, portMAX_DELAY);
    f(v_);
    ver_++;
    xSemaphoreGive(mu_);
  }
  template <class F> void read(F f) const {
    xSemaphoreTake(mu_, portMAX_DELAY);
    f(v_);
    xSemaphoreGive(mu_);
  }
  uint32_t version() const { return ver_; }

 private:
  SemaphoreHandle_t mu_;
  T v_{};
  std::atomic<uint32_t> ver_{0};
};

// DX ring size. The DX table keeps 5 LV_MEM cells per spot (8 B header + text, 4-aligned, + up to 8 B allocator
// overhead: about 120-150 B per spot with the row arrays), so 75 spots stay under 12 KB of LV_MEM (100 would be 14 KB).
constexpr size_t kDxSpots = 75;
constexpr size_t kAprsStations = 30;
using AprsRing = RingBuffer<AprsStation, kAprsStations>;
constexpr size_t kAprsMessages = 20;
using AprsMsgRing = RingBuffer<AprsMessage, kAprsMessages>;  // index 0 = newest

struct DataStore {
  Guarded<RingBuffer<Spot, kDxSpots>> dx;
  Guarded<RingBuffer<Spot, 30>> pota, hamalert;
  Guarded<AprsRing> aprs;
  Guarded<AprsMsgRing> aprsMsg;
  Guarded<PropagationData> prop;
  Guarded<WeatherData> weather;
  Guarded<WeatherData> weatherOm;
  Guarded<NetStatus> net;
};
// Heap-allocated at startup: ~28 KB of static DRAM would not fit next to WiFi and LVGL.
extern DataStore* g_storePtr;
#define g_store (*g_storePtr)
// Call first thing in setup(): a global constructor runs before the heap has its full DRAM and cannot fit 22 KB.
void dataStoreInit();
