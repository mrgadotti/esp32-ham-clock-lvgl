#pragma once
#include <stdint.h>

void memLogTick(uint32_t now);  // prints [MEM] every 10 s
void memLogSetLvFree(uint32_t bytes);  // UI task publishes lv_mem_monitor free bytes
uint32_t memLogLvFree();
void memLogSetTasks(void* net, void* ui, void* uart);  // task handles for the stack watermarks

// Largest contiguous block malloc can serve. ESP.getMaxAllocHeap() also counts the IRAM heap, so it over-reports.
uint32_t memLargest8();
