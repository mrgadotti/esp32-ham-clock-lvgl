#include <unity.h>
#include "tls_policy.h"
void setUp() {} void tearDown() {}
void test_go_when_block_is_big_enough() {
  TEST_ASSERT_TRUE(tlsDecide(kTlsMinBlock, 0) == TlsDecision::Go);
  TEST_ASSERT_TRUE(tlsDecide(34000, 0) == TlsDecision::Go);
}
void test_defers_then_goes_anyway() {
  for (uint8_t d = 0; d < kTlsMaxDeferrals; d++) TEST_ASSERT_TRUE(tlsDecide(kTlsMinBlock - 1, d) == TlsDecision::Defer);
  TEST_ASSERT_TRUE(tlsDecide(1000, kTlsMaxDeferrals) == TlsDecision::Go);  // never blocks for good
  TEST_ASSERT_TRUE(tlsDecide(0, 255) == TlsDecision::Go);
}
void test_quick_retry_only_for_alloc_failure_once() {
  TEST_ASSERT_TRUE(tlsQuickRetry(-32512, false));
  TEST_ASSERT_FALSE(tlsQuickRetry(-32512, true));
  TEST_ASSERT_FALSE(tlsQuickRetry(-76, false));
  TEST_ASSERT_FALSE(tlsQuickRetry(0, false));
}
// A 20 KB heap never recovers: 5 deferrals (2 s apart), then Go on the 6th decision at ~10 s.
void test_gate_worst_case_goes_after_ten_seconds() {
  TlsGate g;
  uint32_t t = 100000, t0 = t;
  int defers = 0;
  for (;; t += 20) {  // net task ticks every 20 ms
    TlsAction a = tlsGateStep(g, t, 20 * 1024);
    if (a == TlsAction::Defer) defers++;
    if (a == TlsAction::Go) break;
    TEST_ASSERT_TRUE(t - t0 < 11000);
  }
  TEST_ASSERT_EQUAL(5, defers);
  TEST_ASSERT_EQUAL_UINT32(10000, t - t0);
  TEST_ASSERT_EQUAL_UINT8(0, g.deferrals);  // reset on Go
}
void test_gate_go_immediately_with_a_big_block() {
  TlsGate g;
  TEST_ASSERT_TRUE(tlsGateStep(g, 5, 40000) == TlsAction::Go);
}
// The old check `(int32_t)(now - notBefore) < 0` blocked for 24.8 days once millis() passed 2^31 with notBefore = 0.
void test_gate_survives_millis_crossing_2_pow_31_and_2_pow_32() {
  const uint32_t times[] = {0x7FFFFFF0u, 0x80000000u, 0x80000010u, 0xFFFFFFF0u, 0u, 20u};
  for (uint32_t now : times) {
    TlsGate fresh;  // zero state: nothing stale to wait for
    TEST_ASSERT_TRUE(tlsGateStep(fresh, now, 34000) == TlsAction::Go);
    TlsGate stale;  // a deferral from ~2^31 ms ago
    stale.deferring = true; stale.deferredAt = now - 0x80000001u;
    TEST_ASSERT_TRUE(tlsGateStep(stale, now, 34000) == TlsAction::Go);
  }
}
void test_gate_window_across_the_wrap() {
  TlsGate g;
  uint32_t t = 0xFFFFFC00u;  // 1 s before the wrap
  TEST_ASSERT_TRUE(tlsGateStep(g, t, 1000) == TlsAction::Defer);
  TEST_ASSERT_TRUE(tlsGateStep(g, t + 1999, 40000) == TlsAction::Wait);  // wraps to 0x3CF
  TEST_ASSERT_TRUE(tlsGateStep(g, t + 2000, 40000) == TlsAction::Go);
}
int main() { UNITY_BEGIN(); RUN_TEST(test_go_when_block_is_big_enough); RUN_TEST(test_defers_then_goes_anyway); RUN_TEST(test_quick_retry_only_for_alloc_failure_once); RUN_TEST(test_gate_worst_case_goes_after_ten_seconds); RUN_TEST(test_gate_go_immediately_with_a_big_block); RUN_TEST(test_gate_survives_millis_crossing_2_pow_31_and_2_pow_32); RUN_TEST(test_gate_window_across_the_wrap); return UNITY_END(); }
