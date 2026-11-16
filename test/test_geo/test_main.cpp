#include <unity.h>
#include <string.h>
#include "geo.h"
void setUp() {} void tearDown() {}
void test_locator() { char o[7]; TEST_ASSERT_TRUE(latLonToLocator(-23.55, -46.63, o)); TEST_ASSERT_EQUAL_STRING("GG66qk", o); }
void test_locator_bad() { char o[7]; TEST_ASSERT_FALSE(latLonToLocator(91, 0, o)); }
void test_loc_to_ll() { double a, b; TEST_ASSERT_TRUE(locatorToLatLon("GG66qk", a, b)); TEST_ASSERT_FLOAT_WITHIN(0.05, -23.55, a); TEST_ASSERT_FLOAT_WITHIN(0.05, -46.63, b);
  TEST_ASSERT_TRUE(locatorToLatLon("GG66", a, b)); TEST_ASSERT_FLOAT_WITHIN(0.001, -23.5, a); TEST_ASSERT_FALSE(locatorToLatLon("G", a, b)); }
void test_dist() { TEST_ASSERT_FLOAT_WITHIN(0.1, 111.19, distanceKm(0, 0, 0, 1)); }
void test_bearing() { TEST_ASSERT_FLOAT_WITHIN(0.1, 0, bearingDeg(0, 0, 1, 0)); TEST_ASSERT_FLOAT_WITHIN(0.1, 90, bearingDeg(0, 0, 0, 1)); }
int main() { UNITY_BEGIN(); RUN_TEST(test_locator); RUN_TEST(test_locator_bad); RUN_TEST(test_loc_to_ll); RUN_TEST(test_dist); RUN_TEST(test_bearing); return UNITY_END(); }
