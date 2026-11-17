#ifdef PERF_PROF
#include <Arduino.h>
#include <stdlib.h>
#include "perf_prof.h"

namespace {
struct Ent { const char* n; uint32_t self, calls; };
Ent ents[96];
int nEnts;
struct Fr { int idx; uint32_t t0, child; };
Fr st[40];
int sp;
int find(const char* n) {
  for (int i = 0; i < nEnts; i++) if (ents[i].n == n) return i;
  if (nEnts < 96) { ents[nEnts] = {n, 0, 0}; return nEnts++; }
  return 0;
}
}  // namespace

extern "C" void perfProfBegin(const char* name) {
  if (sp >= 40) { sp++; return; }
  st[sp++] = {find(name), (uint32_t)micros(), 0};
}
extern "C" void perfProfEnd(const char*) {
  if (sp > 40) { sp--; return; }
  if (sp <= 0) return;
  Fr f = st[--sp];
  uint32_t d = (uint32_t)micros() - f.t0;
  ents[f.idx].self += d - f.child;
  ents[f.idx].calls++;
  if (sp > 0) st[sp - 1].child += d;
}
extern "C" void perfProfDump(void) {
  bool used[96] = {};
  for (int k = 0; k < 14; k++) {
    int b = -1;
    for (int i = 0; i < nEnts; i++) if (!used[i] && (b < 0 || ents[i].self > ents[b].self)) b = i;
    if (b < 0 || ents[b].self < 1000) break;
    used[b] = true;
    Serial.printf("PROF %-34s self_ms=%u calls=%u\n", ents[b].n, ents[b].self / 1000, ents[b].calls);
  }
  for (int i = 0; i < nEnts; i++) ents[i].self = ents[i].calls = 0;
}
#endif
