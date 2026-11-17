#pragma once
#include <WiFiClient.h>
#include <stddef.h>

// Line splitter shared by the DX, APRS-IS and HamAlert sockets. Never blocks: reads only what is already buffered.
// CR and LF both end a line; empty lines are skipped; a line longer than the buffer is dropped up to its newline.
class LineReader {
 public:
  // asciiOnly keeps 32..126 only (telnet option bytes and control characters are dropped).
  LineReader(char* buf, size_t cap, bool asciiOnly) : buf_(buf), cap_(cap), ascii_(asciiOnly) {}
  void reset() { len_ = 0; discard_ = false; }
  // Reads up to `budget` bytes, calls onLine(const char*) per complete line, returns the bytes read.
  template <class F> size_t pump(WiFiClient& c, size_t budget, F onLine) {
    size_t n = 0;
    while (n < budget && c.available()) {
      int ch = c.read();
      if (ch < 0) break;
      n++;
      if (ch == '\n' || ch == '\r') {
        if (len_ && !discard_) { buf_[len_] = 0; onLine((const char*)buf_); }
        reset();
      } else if (!ascii_ || (ch >= 32 && ch < 127)) {
        if (len_ < cap_ - 1) buf_[len_++] = (char)ch;
        else discard_ = true;
      }
    }
    return n;
  }

 private:
  char* buf_;
  size_t cap_, len_ = 0;
  bool ascii_, discard_ = false;
};
