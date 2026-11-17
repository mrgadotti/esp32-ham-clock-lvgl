#include "tls_policy.h"

TlsDecision tlsDecide(uint32_t largest, uint8_t deferrals) {
  return largest >= kTlsMinBlock || deferrals >= kTlsMaxDeferrals ? TlsDecision::Go : TlsDecision::Defer;
}

bool tlsQuickRetry(int err, bool alreadyRetried) { return err == kMbedtlsAllocFailed && !alreadyRetried; }
