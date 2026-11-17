#pragma once
#include <stdint.h>

// TLS heap policy (M-06). A handshake needs ~2 x 16.6 KB I/O buffers plus state; idle largest block on the device is
// ~34 KB and fetches work there, so the threshold only avoids starting while something transient holds the heap.
// It never blocks for good: after kTlsMaxDeferrals deferrals the poll goes anyway.
constexpr uint32_t kTlsMinBlock = 28u * 1024;
constexpr uint8_t kTlsMaxDeferrals = 5;
constexpr uint32_t kTlsDeferMs = 2000;
constexpr uint32_t kTlsQuickRetryMs = 4000;
constexpr int kMbedtlsAllocFailed = -0x7F00;  // MBEDTLS_ERR_SSL_ALLOC_FAILED, logged as -32512

enum class TlsDecision : uint8_t { Go, Defer };
TlsDecision tlsDecide(uint32_t largestFreeBlock, uint8_t deferralsSoFar);

// One quick retry (kTlsQuickRetryMs) after an SSL allocation failure, instead of waiting for the next full interval.
bool tlsQuickRetry(int mbedtlsError, bool alreadyRetried);
