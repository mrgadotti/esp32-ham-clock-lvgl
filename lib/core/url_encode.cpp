#include "url_encode.h"
#include <stdio.h>

void urlEncode(const char* in, char* out, size_t cap) {
  if (!cap) return;
  size_t o = 0;
  for (; *in && o + 4 < cap; ++in) {
    unsigned char c = (unsigned char)*in;
    bool plain = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '-' || c == '_' || c == '.' || c == '~';
    if (plain) out[o++] = (char)c;
    else o += snprintf(out + o, 4, "%%%02X", c);
  }
  out[o] = 0;
}
