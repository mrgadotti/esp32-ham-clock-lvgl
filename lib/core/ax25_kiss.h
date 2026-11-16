#pragma once
#include <stddef.h>
#include <stdint.h>

struct KissRx {
  bool inFrame = false;
  bool esc = false;
  bool done = false;   // last call completed a frame; buf/len stay valid until the next byte
  bool haveCmd = false;
  uint8_t cmd = 0;     // KISS command byte (port<<4 | cmd) of the current frame
  uint16_t len = 0;
  uint8_t buf[600];    // AX.25 frame without the KISS command byte
};

// KISS frame: C0 00 <escaped AX.25 UI frame> C0. Returns bytes written, 0 on error.
size_t kissEncodeUiFrame(const char* src, const char* dst, const char* const* path, uint8_t nPath,
                         const char* info, uint8_t* out, size_t cap);
// Feed one byte; true when a complete frame is now in st.buf[0..st.len).
bool kissDeframe(uint8_t byte, KissRx& st);
// AX.25 UI frame -> "SRC>DST,DIGI*:info" (printable ASCII only).
bool ax25ToTnc2(const uint8_t* frame, size_t len, char* out, size_t cap);
// "!DDMM.mmN<table>DDDMM.mmW<code><comment>"
bool formatAprsPosition(double lat, double lon, char symTable, char symCode, const char* comment,
                        char* out, size_t cap);
bool aprsTxCallValid(const char* call);
