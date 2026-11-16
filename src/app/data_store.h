#pragma once
#include <Arduino.h>
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
  volatile uint32_t ver_ = 0;
};

struct DataStore {
  Guarded<RingBuffer<Spot, 100>> dx;
  Guarded<RingBuffer<Spot, 30>> pota, hamalert;
  Guarded<RingBuffer<AprsStation, 30>> aprs;
  Guarded<PropagationData> prop;
  Guarded<WeatherData> weather;
  Guarded<GpsState> gps;
  Guarded<NetStatus> net;
};
// Heap-allocated at startup: ~28 KB of static DRAM would not fit next to WiFi and LVGL.
extern DataStore& g_store;
