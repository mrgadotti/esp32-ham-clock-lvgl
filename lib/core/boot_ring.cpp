#include "boot_ring.h"

#include "text_cut.h"

void BootLogRing::push(const char* s) {
  char* d = line[next];
  textCut(d, kBootLogLen, s, kBootLogLen - 1);
  for (char* p = d; *p; p++)
    if ((unsigned char)*p < 32) *p = ' ';
  next = (next + 1) % kBootLogLines;
  if (count < kBootLogLines) count++;
  ver++;
}
