#include "mem_log.h"

#include <Arduino.h>
#include <atomic>

static TaskHandle_t hNet, hUi, hUart;
void memLogSetTasks(void* n, void* u, void* x) { hNet = (TaskHandle_t)n; hUi = (TaskHandle_t)u; hUart = (TaskHandle_t)x; }

uint32_t memLargest8() { return (uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT); }

static std::atomic<uint32_t> lvFree{0};

void memLogSetLvFree(uint32_t b) { lvFree = b; }
uint32_t memLogLvFree() { return lvFree; }

void memLogTick(uint32_t now) {
  static uint32_t last = 0;
  if (now - last < 10000) return;
  last = now;
  Serial.printf("[MEM] free=%u minfree=%u largest=%u lvfree=%u netstk=%u uistk=%u uartstk=%u\n", (unsigned)ESP.getFreeHeap(),
                (unsigned)ESP.getMinFreeHeap(), (unsigned)memLargest8(), (unsigned)lvFree.load(),
                hNet ? (unsigned)uxTaskGetStackHighWaterMark(hNet) : 0u, hUi ? (unsigned)uxTaskGetStackHighWaterMark(hUi) : 0u,
                hUart ? (unsigned)uxTaskGetStackHighWaterMark(hUart) : 0u);
}
