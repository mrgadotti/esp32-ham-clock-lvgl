#include "tls_policy.h"

TlsDecision tlsDecide(uint32_t largest, uint8_t deferrals) {
  return largest >= kTlsMinBlock || deferrals >= kTlsMaxDeferrals ? TlsDecision::Go : TlsDecision::Defer;
}

bool tlsGateWaiting(const TlsGate& g, uint32_t now) { return g.deferring && now - g.deferredAt < kTlsDeferMs; }

TlsAction tlsGateStep(TlsGate& g, uint32_t now, uint32_t largest) {
  if (tlsGateWaiting(g, now)) return TlsAction::Wait;
  g.deferring = false;
  if (tlsDecide(largest, g.deferrals) == TlsDecision::Go) {
    g.deferrals = 0;
    return TlsAction::Go;
  }
  g.deferrals++;
  g.deferring = true;
  g.deferredAt = now;
  return TlsAction::Defer;
}

bool tlsQuickRetry(int err, bool alreadyRetried) { return err == kMbedtlsAllocFailed && !alreadyRetried; }
