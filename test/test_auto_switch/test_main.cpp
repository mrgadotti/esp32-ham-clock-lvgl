#include <unity.h>
#include "gesture.h"
void setUp() {} void tearDown() {}
// Auto-switch never goes to Settings: after the last carousel screen it wraps to the first one.
void test_auto_switch_wraps_past_settings() {
  const uint8_t order[12] = {1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0}; uint16_t en = (1u << 1) | (1u << 2) | (1u << 3);
  TEST_ASSERT_EQUAL(2, nextAutoSwitchScreen(order, en, 1));
  TEST_ASSERT_EQUAL(3, nextAutoSwitchScreen(order, en, 2));
  TEST_ASSERT_EQUAL(1, nextAutoSwitchScreen(order, en, 3)); }
void test_auto_switch_skips_disabled_and_removed() {
  const uint8_t order[12] = {1, 13, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0}; uint16_t en = (1u << 1) | (1u << 3) | (1u << 10);
  TEST_ASSERT_EQUAL(3, nextAutoSwitchScreen(order, en, 1));
  TEST_ASSERT_EQUAL(1, nextAutoSwitchScreen(order, en, 3)); }
// A disabled or unknown current screen (or Settings itself) restarts at the first carousel screen.
void test_auto_switch_from_outside_carousel() {
  const uint8_t order[12] = {1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0}; uint16_t en = (1u << 1) | (1u << 3);
  TEST_ASSERT_EQUAL(1, nextAutoSwitchScreen(order, en, 2));
  TEST_ASSERT_EQUAL(1, nextAutoSwitchScreen(order, en, scr::Settings)); }
// Nothing to switch to: stay on the current screen.
void test_auto_switch_stays_when_alone() {
  const uint8_t order[12] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  TEST_ASSERT_EQUAL(1, nextAutoSwitchScreen(order, 1u << 1, 1));
  TEST_ASSERT_EQUAL(1, nextAutoSwitchScreen(order, 0, scr::Settings)); }  // Clock is always enabled
int main() {
  UNITY_BEGIN();
  RUN_TEST(test_auto_switch_wraps_past_settings);
  RUN_TEST(test_auto_switch_skips_disabled_and_removed);
  RUN_TEST(test_auto_switch_from_outside_carousel);
  RUN_TEST(test_auto_switch_stays_when_alone);
  return UNITY_END(); }
