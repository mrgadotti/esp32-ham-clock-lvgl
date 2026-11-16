#pragma once
#include <stdint.h>

void memLogTick(uint32_t now);  // prints [MEM] every 10 s
void memLogSetLvFree(uint32_t bytes);  // UI task publishes lv_mem_monitor free bytes
uint32_t memLogLvFree();
