#include <unity.h>
#include "backoff.h"
void setUp() {} void tearDown() {}
void test_backoff() { TEST_ASSERT_EQUAL_UINT32(5000, backoffMs(0)); TEST_ASSERT_EQUAL_UINT32(10000, backoffMs(1));
  TEST_ASSERT_EQUAL_UINT32(80000, backoffMs(4)); TEST_ASSERT_EQUAL_UINT32(300000, backoffMs(10)); TEST_ASSERT_EQUAL_UINT32(300000, backoffMs(255)); }
int main() { UNITY_BEGIN(); RUN_TEST(test_backoff); return UNITY_END(); }
