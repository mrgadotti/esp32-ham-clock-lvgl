#include "mem_log.h"

#include <Arduino.h>
#include <atomic>

static std::atomic<uint32_t> lvFree{0};

void memLogSetLvFree(uint32_t b) { lvFree = b; }
uint32_t memLogLvFree() { return lvFree; }

void memLogTick(uint32_t now) {
  static uint32_t last = 0;
  if (now - last < 10000) return;
  last = now;
  Serial.printf("[MEM] free=%u minfree=%u largest=%u lvfree=%u\n", (unsigned)ESP.getFreeHeap(),
                (unsigned)ESP.getMinFreeHeap(), (unsigned)ESP.getMaxAllocHeap(), (unsigned)lvFree.load());
}
