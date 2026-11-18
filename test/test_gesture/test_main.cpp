#include <unity.h>
#include "gesture.h"
#include <initializer_list>
void setUp() {} void tearDown() {}
void test_gesture() {
  TEST_ASSERT_EQUAL(Gesture::SwipeLeft, classifyGesture(-60, 10)); TEST_ASSERT_EQUAL(Gesture::SwipeRight, classifyGesture(60, 10));
  TEST_ASSERT_EQUAL(Gesture::None, classifyGesture(30, 5)); TEST_ASSERT_EQUAL(Gesture::None, classifyGesture(-60, 50));
  TEST_ASSERT_EQUAL(Gesture::SwipeRight, classifyGesture(45, 5)); TEST_ASSERT_EQUAL(Gesture::SwipeLeft, classifyGesture(-45, 5));
  TEST_ASSERT_EQUAL(Gesture::None, classifyGesture(60, 30)); TEST_ASSERT_EQUAL(Gesture::None, classifyGesture(40, 0)); }
void test_removed_ids_are_not_carousel() {
  for (uint8_t id : {0, 10, 11, 13, 14, 15, 200}) TEST_ASSERT_FALSE(isCarouselScreen(id));
  for (uint8_t id : {1, 2, 3, 4, 5, 6, 7, 8, 9, 12}) TEST_ASSERT_TRUE(isCarouselScreen(id));
  const uint8_t order[12] = {1, 10, 11, 13, 2, 0, 0, 0, 0, 0, 0, 0};
  TEST_ASSERT_EQUAL(2, nextScreen(order, 0xFFFF, 1, 1)); TEST_ASSERT_EQUAL(1, nextScreen(order, 0xFFFF, 2, 1));
  for (uint8_t id : {13, 10, 11, 14}) TEST_ASSERT_EQUAL_UINT8(scr::HamClock, resolveStartScreen(id, 0xFFFF)); }
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
static void test_resolve_start() {
  uint16_t all = 0xFFFF;
  TEST_ASSERT_EQUAL_UINT8(scr::Weather, resolveStartScreen(scr::Weather, all));
  TEST_ASSERT_EQUAL_UINT8(scr::HamClock, resolveStartScreen(scr::Settings, all));
  TEST_ASSERT_EQUAL_UINT8(scr::HamClock, resolveStartScreen(scr::Weather, all & ~(1u << scr::Weather)));
  TEST_ASSERT_EQUAL_UINT8(scr::HamClock, resolveStartScreen(0, all));
  TEST_ASSERT_EQUAL_UINT8(scr::HamClock, resolveStartScreen(99, all));
}
int main() { UNITY_BEGIN(); RUN_TEST(test_gesture); RUN_TEST(test_next_skips_disabled_and_wraps); RUN_TEST(test_removed_ids_are_not_carousel); RUN_TEST(test_clock_always_enabled_and_never_settings); RUN_TEST(test_resolve_start); return UNITY_END(); }
