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
int main() { UNITY_BEGIN(); RUN_TEST(test_go_when_block_is_big_enough); RUN_TEST(test_defers_then_goes_anyway); RUN_TEST(test_quick_retry_only_for_alloc_failure_once); return UNITY_END(); }
