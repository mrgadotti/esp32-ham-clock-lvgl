#include <unity.h>
#include "gps_time.h"
void setUp() {} void tearDown() {}
void test_days_from_civil() {
  TEST_ASSERT_EQUAL_INT64(0, daysFromCivil(1970, 1, 1)); TEST_ASSERT_EQUAL_INT64(365, daysFromCivil(1971, 1, 1));
  TEST_ASSERT_EQUAL_INT64(11016, daysFromCivil(2000, 2, 29));  // leap day of a 400-year year
  TEST_ASSERT_EQUAL_INT64(11017, daysFromCivil(2000, 3, 1)); TEST_ASSERT_EQUAL_INT64(-1, daysFromCivil(1969, 12, 31));
  TEST_ASSERT_EQUAL_INT64(20453, daysFromCivil(2025, 12, 31)); }
void test_gps_to_unix() {
  int64_t t = 0;
  TEST_ASSERT_TRUE(gpsToUnix(160126, 123456, t)); TEST_ASSERT_EQUAL_INT64(1768566896LL, t);  // 2026-01-16 12:34:56 UTC
  TEST_ASSERT_TRUE(gpsToUnix(290224, 235959, t)); TEST_ASSERT_EQUAL_INT64(1709251199LL, t);  // 2024-02-29 leap day
  TEST_ASSERT_TRUE(gpsToUnix(10124, 1, t)); TEST_ASSERT_EQUAL_INT64(1704067201LL, t); }
void test_gps_to_unix_rejects() {
  int64_t t = 77;
  TEST_ASSERT_FALSE(gpsToUnix(0, 123456, t)); TEST_ASSERT_FALSE(gpsToUnix(160126, 0, t));  // unknown
  TEST_ASSERT_FALSE(gpsToUnix(311223, 120000, t));                                         // year 2023 < 2024
  TEST_ASSERT_FALSE(gpsToUnix(161326, 120000, t)); TEST_ASSERT_FALSE(gpsToUnix(320126, 120000, t));  // bad month/day
  TEST_ASSERT_FALSE(gpsToUnix(160026, 120000, t)); TEST_ASSERT_FALSE(gpsToUnix(160126, 240000, t));
  TEST_ASSERT_FALSE(gpsToUnix(160126, 126000, t)); TEST_ASSERT_FALSE(gpsToUnix(160126, 120060, t));
  TEST_ASSERT_EQUAL_INT64(77, t); }  // untouched on failure
int main() { UNITY_BEGIN(); RUN_TEST(test_days_from_civil); RUN_TEST(test_gps_to_unix); RUN_TEST(test_gps_to_unix_rejects); return UNITY_END(); }
