#pragma once
#include <WiFiClient.h>
#include <stdint.h>

// Non-blocking TCP connect for the net task. WiFiClient::connect() blocks the task for the whole connect timeout and
// its host lookup can wait 15 s (not bounded by that timeout). Here DNS is an lwIP callback and connect() runs on a
// non-blocking socket polled with select(), so every call returns at once and both phases have their own timeout.
// One object per socket; static storage only (an lwIP callback may still point at it after cancel()).
class AsyncConnect {
 public:
  enum State : uint8_t { Idle, Dns, Connecting, Done, Failed };
  void start(const char* host, uint16_t port, uint32_t now);
  // Call every tick while busy(). Done: `out` is now the connected client. Done/Failed are returned once, then Idle.
  State poll(uint32_t now, WiFiClient& out);
  void cancel();
  bool busy() const { return st_ == Dns || st_ == Connecting; }

  // Written by the lwIP callback (tcpip thread); public only so the file-local callback can reach them.
  volatile uint8_t dnsState_ = 0;  // 0 pending, 1 resolved, 2 failed
  volatile uint32_t ip_ = 0;       // network order
  char host_[64];

 private:
  void connectNow(uint32_t now);
  State st_ = Idle;
  uint16_t port_ = 0;
  int fd_ = -1;
  uint32_t t0_ = 0;
};
