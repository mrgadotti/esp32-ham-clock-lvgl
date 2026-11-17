#pragma once
#include <stdint.h>

void memLogTick(uint32_t now);  // prints [MEM] every 10 s
void memLogSetLvFree(uint32_t bytes);  // UI task publishes lv_mem_monitor free bytes
uint32_t memLogLvFree();
void memLogSetTasks(void* net, void* ui, void* uart);  // task handles for the stack watermarks

// Largest contiguous block malloc can serve. ESP.getMaxAllocHeap() also counts the IRAM heap, so it over-reports.
uint32_t memLargest8();

// Heap diagnostics around a service's first connect (M-06): one [MEM] line with free / largest block / block counts.
// Compile out with -DMEM_DIAG=0. Each MEM_DIAG call site prints once.
#ifndef MEM_DIAG
#define MEM_DIAG 1
#endif
void memLogHeap(const char* tag);
#if MEM_DIAG
#define MEM_DIAG_ONCE(tag) do { static bool once_; if (!once_) { once_ = true; memLogHeap(tag); } } while (0)
#else
#define MEM_DIAG_ONCE(tag) do {} while (0)
#endif
