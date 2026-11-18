#include <unity.h>
#include <stdio.h>
#include <string.h>
#include "aprs_sort.h"
#include "nearest_index.h"

void setUp() {}
void tearDown() {}

static AprsStation st(const char* call, bool ll, int16_t km) {
  AprsStation s{};
  snprintf(s.call, sizeof s.call, "%s", call);
  s.hasLatLon = ll;
  s.distKm = km;
  return s;
}

void test_time_never_reorders() {
  AprsStation a = st("A", true, 1), b = st("B", true, 2);
  TEST_ASSERT_FALSE(aprsBefore(a, b, APRS_SORT_TIME));
  TEST_ASSERT_FALSE(aprsBefore(b, a, APRS_SORT_TIME));
}
void test_call_ascending() {
  AprsStation a = st("DL1ABC", false, -1), b = st("PY2XYZ", false, -1);
  TEST_ASSERT_TRUE(aprsBefore(a, b, APRS_SORT_CALL));
  TEST_ASSERT_FALSE(aprsBefore(b, a, APRS_SORT_CALL));
  TEST_ASSERT_FALSE(aprsBefore(a, a, APRS_SORT_CALL));
}
void test_dist_ascending_unknown_last() {
  AprsStation near = st("A", true, 5), far = st("B", true, 50), noPos = st("C", false, 1), neg = st("D", true, -1);
  TEST_ASSERT_TRUE(aprsBefore(near, far, APRS_SORT_DIST));
  TEST_ASSERT_FALSE(aprsBefore(far, near, APRS_SORT_DIST));
  TEST_ASSERT_TRUE(aprsBefore(far, noPos, APRS_SORT_DIST));
  TEST_ASSERT_FALSE(aprsBefore(noPos, far, APRS_SORT_DIST));
  TEST_ASSERT_TRUE(aprsBefore(far, neg, APRS_SORT_DIST));
  TEST_ASSERT_FALSE(aprsBefore(noPos, neg, APRS_SORT_DIST));  // both unknown: equal
  TEST_ASSERT_FALSE(aprsBefore(neg, noPos, APRS_SORT_DIST));
}
void test_nearest_index() {
  const int32_t v[] = {10, 15, 30, 60};
  TEST_ASSERT_EQUAL(0, nearestIndex(v, 4, 0));
  TEST_ASSERT_EQUAL(1, nearestIndex(v, 4, 16));
  TEST_ASSERT_EQUAL(2, nearestIndex(v, 4, 40));
  TEST_ASSERT_EQUAL(3, nearestIndex(v, 4, 1000));
  TEST_ASSERT_EQUAL(0, nearestIndex(v, 4, 12));  // 12 is 2 from 10, 3 from 15
  TEST_ASSERT_EQUAL(0, nearestIndex(v, 0, 5));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_time_never_reorders);
  RUN_TEST(test_call_ascending);
  RUN_TEST(test_dist_ascending_unknown_last);
  RUN_TEST(test_nearest_index);
  return UNITY_END();
}
