#include <unity.h>
#include "gesture.h"
void setUp() {} void tearDown() {}
void test_gesture() {
  TEST_ASSERT_EQUAL(Gesture::SwipeLeft, classifyGesture(-60, 10)); TEST_ASSERT_EQUAL(Gesture::SwipeRight, classifyGesture(60, 10));
  TEST_ASSERT_EQUAL(Gesture::None, classifyGesture(30, 5)); TEST_ASSERT_EQUAL(Gesture::None, classifyGesture(-60, 50));
  TEST_ASSERT_EQUAL(Gesture::SwipeRight, classifyGesture(45, 5)); TEST_ASSERT_EQUAL(Gesture::SwipeLeft, classifyGesture(-45, 5));
  TEST_ASSERT_EQUAL(Gesture::None, classifyGesture(60, 30)); TEST_ASSERT_EQUAL(Gesture::None, classifyGesture(40, 0)); }
void test_next_skips_disabled_and_wraps() {
  const uint8_t order[12] = {1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0}; uint16_t en = (1u << 1) | (1u << 3);
  TEST_ASSERT_EQUAL(3, nextScreen(order, en, 1, 1)); TEST_ASSERT_EQUAL(scr::Settings, nextScreen(order, en, 3, 1));
  TEST_ASSERT_EQUAL(1, nextScreen(order, en, scr::Settings, 1)); TEST_ASSERT_EQUAL(scr::Settings, nextScreen(order, en, 1, -1)); }
int main() { UNITY_BEGIN(); RUN_TEST(test_gesture); RUN_TEST(test_next_skips_disabled_and_wraps); return UNITY_END(); }
