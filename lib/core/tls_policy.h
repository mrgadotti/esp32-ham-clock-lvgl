#pragma once
#include <stdint.h>

// TLS heap policy (M-06). A handshake needs ~2 x 16.6 KB I/O buffers plus state. Measured on the board: the largest
// block is 61 KB then 35 KB while booting, but only ~32.7 KB once screen 1 exists, and that is too fragmented for a
// handshake (-32512). The real fix is ordering (the first HTTPS round runs before screen 1, see ui_manager), so the
// threshold stays a transient-holder guard only; raising it would just burn the deferrals after the UI is up.
// It never blocks for good: after kTlsMaxDeferrals deferrals the poll goes anyway.
constexpr uint32_t kTlsMinBlock = 33u * 1024;  // measured: handshake failed at 30708 and once at 32756, never at >= 34804
constexpr uint8_t kTlsMaxDeferrals = 5;
constexpr uint32_t kTlsDeferMs = 2000;
constexpr uint32_t kTlsRetryMinMs = 4000, kTlsRetryMaxMs = 60000;  // allocation-failure retries: 4, 8, 16, 32, 60, 60 ... s
constexpr int kMbedtlsAllocFailed = -0x7F00;  // MBEDTLS_ERR_SSL_ALLOC_FAILED, logged as -32512

enum class TlsDecision : uint8_t { Go, Defer };
TlsDecision tlsDecide(uint32_t largestFreeBlock, uint8_t deferralsSoFar);

// Deferral state machine for the poll gate. All time compares are unsigned differences against a short window, so
// they survive millis() wrapping; `deferring` (not a stale timestamp) says whether a window is open.
struct TlsGate {
  bool deferring = false;
  uint32_t deferredAt = 0;
  uint8_t deferrals = 0;
};
enum class TlsAction : uint8_t { Wait, Go, Defer };  // Wait: inside a defer window; Defer: new deferral (log it)
bool tlsGateWaiting(const TlsGate& g, uint32_t now);  // true: Wait, no need to measure the heap
TlsAction tlsGateStep(TlsGate& g, uint32_t now, uint32_t largestFreeBlock);

// After an SSL allocation failure retry soon (and again, doubling to 60 s) instead of waiting for the next full interval.
// `failures` counts consecutive allocation failures including this one (>= 1). Other errors: 0 = use the normal interval.
uint32_t tlsAllocRetryMs(int mbedtlsError, uint8_t failures);
