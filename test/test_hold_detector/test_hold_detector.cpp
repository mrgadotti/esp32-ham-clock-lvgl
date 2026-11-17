#include <unity.h>
#include "hold_detector.h"
void setUp() {} void tearDown() {}
void test_fires_at_15s() {
  HoldDetector h; h.press(100, 100, 1000);
  TEST_ASSERT_FALSE(h.update(1000 + 14999));
  TEST_ASSERT_TRUE(h.update(1000 + 15000));
}
void test_release_cancels() {
  HoldDetector h; h.press(1, 1, 0); h.release();
  TEST_ASSERT_FALSE(h.update(20000));
}
void test_move_beyond_slop_cancels() {
  HoldDetector h; h.press(100, 100, 0);
  h.move(110, 110);  // within slop
  TEST_ASSERT_TRUE(h.active());
  h.move(100, 121);
  TEST_ASSERT_FALSE(h.update(20000));
}
void test_fires_once_and_rearms() {
  HoldDetector h; h.press(0, 0, 0);
  TEST_ASSERT_TRUE(h.update(15000));
  TEST_ASSERT_FALSE(h.update(16000));
  h.release(); h.press(0, 0, 30000);
  TEST_ASSERT_FALSE(h.update(44999));
  TEST_ASSERT_TRUE(h.update(45000));
}
void test_millis_wrap() {
  HoldDetector h; h.press(0, 0, 0xFFFFFFFFu - 5000);
  TEST_ASSERT_TRUE(h.update(10000));
}
int main() { UNITY_BEGIN(); RUN_TEST(test_fires_at_15s); RUN_TEST(test_release_cancels); RUN_TEST(test_move_beyond_slop_cancels);
  RUN_TEST(test_fires_once_and_rearms); RUN_TEST(test_millis_wrap); return UNITY_END(); }
