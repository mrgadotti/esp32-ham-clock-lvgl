#pragma once
// PERF_TEST harness, called from the UI task loop around lv_timer_handler(). Compile with -DPERF_TEST.
void perfTestInit();
void perfBeforeHandler();  // right before lv_timer_handler()
void perfAfterHandler();   // right after it: accounts the handler time and drives the scroll phases
