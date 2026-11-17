#include "prop_level.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

PropCond propCondOf(const char* s) {
  char u[8];
  size_t i = 0;
  for (; s[i] && i < sizeof u - 1; i++) u[i] = toupper((unsigned char)s[i]);
  u[i] = 0;
  if (strstr(u, "GOOD")) return PropCond::Good;
  if (strstr(u, "FAIR")) return PropCond::Fair;
  if (strstr(u, "POOR")) return PropCond::Poor;
  return PropCond::Unknown;
}

KLevel kIndexLevel(const char* s) {
  char* end;
  long k = strtol(s, &end, 10);
  if (end == s) return KLevel::None;
  return k < 4 ? KLevel::Low : k < 6 ? KLevel::Mid : KLevel::High;
}
