#include "radar_label.h"

#include <stdio.h>

void formatRadarKm(float v, char* b, size_t n) {
  if (v >= 100) snprintf(b, n, "%dkm", (int)(v + 0.5f));
  else if (v >= 10) snprintf(b, n, "%.1fkm", (double)v);
  else snprintf(b, n, "%.2fkm", (double)v);
}
