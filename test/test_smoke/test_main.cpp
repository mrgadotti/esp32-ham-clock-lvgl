#include <unity.h>

void setUp() {}
void tearDown() {}

void test_smoke() { TEST_ASSERT_EQUAL(1, 1); }

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_smoke);
  return UNITY_END();
}
