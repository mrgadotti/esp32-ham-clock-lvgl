#pragma once
// PERF_PROF builds only: LVGL profiler hooks that accumulate self time per function.
#ifdef __cplusplus
extern "C" {
#endif
void perfProfBegin(const char* name);
void perfProfEnd(const char* name);
void perfProfDump(void);
#ifdef __cplusplus
}
#endif
