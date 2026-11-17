#include <unity.h>
#include "gesture.h"
void setUp() {} void tearDown() {}
void test_gesture() {
  TEST_ASSERT_EQUAL(Gesture::SwipeLeft, classifyGesture(-60, 10)); TEST_ASSERT_EQUAL(Gesture::SwipeRight, classifyGesture(60, 10));
  TEST_ASSERT_EQUAL(Gesture::None, classifyGesture(30, 5)); TEST_ASSERT_EQUAL(Gesture::None, classifyGesture(-60, 50));
  TEST_ASSERT_EQUAL(Gesture::SwipeRight, classifyGesture(45, 5)); TEST_ASSERT_EQUAL(Gesture::SwipeLeft, classifyGesture(-45, 5));
  TEST_ASSERT_EQUAL(Gesture::None, classifyGesture(60, 30)); TEST_ASSERT_EQUAL(Gesture::None, classifyGesture(40, 0)); }
void test_next_skips_removed_matrix() {
  const uint8_t order[12] = {1, 10, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0}; uint16_t en = (1u << 1) | (1u << 3) | (1u << 10);
  TEST_ASSERT_EQUAL(3, nextScreen(order, en, 1, 1)); TEST_ASSERT_EQUAL(1, nextScreen(order, en, 3, 1)); }
void test_next_skips_disabled_and_wraps() {
  const uint8_t order[12] = {1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0}; uint16_t en = (1u << 1) | (1u << 3);
  TEST_ASSERT_EQUAL(3, nextScreen(order, en, 1, 1)); TEST_ASSERT_EQUAL(1, nextScreen(order, en, 3, 1));
  TEST_ASSERT_EQUAL(3, nextScreen(order, en, 1, -1)); TEST_ASSERT_EQUAL(1, nextScreen(order, en, scr::Settings, 1)); }
void test_clock_always_enabled_and_never_settings() {
  const uint8_t order[12] = {1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  TEST_ASSERT_EQUAL(1, nextScreen(order, 0, 1, 1)); TEST_ASSERT_EQUAL(1, nextScreen(order, 0, 1, -1));  // nothing enabled: stays on Clock
  TEST_ASSERT_EQUAL(3, nextScreen(order, 1u << 3, 1, 1)); TEST_ASSERT_EQUAL(1, nextAutoSwitchScreen(order, 1u << 3, 3));
  TEST_ASSERT_EQUAL(2, nextAutoSwitchScreen(order, 0xFFFF, 1));
  for (uint8_t c = 1; c <= 3; ++c) TEST_ASSERT_NOT_EQUAL(scr::Settings, nextScreen(order, 0xFFFF, c, 1)); }
int main() { UNITY_BEGIN(); RUN_TEST(test_gesture); RUN_TEST(test_next_skips_disabled_and_wraps); RUN_TEST(test_next_skips_removed_matrix); RUN_TEST(test_clock_always_enabled_and_never_settings); return UNITY_END(); }
