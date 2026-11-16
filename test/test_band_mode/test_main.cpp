#include <unity.h>
#include "band_mode.h"
void setUp() {} void tearDown() {}
void test_bands() {
  TEST_ASSERT_EQUAL(BAND_40M, bandFromKhz(7074)); TEST_ASSERT_EQUAL(BAND_30M, bandFromKhz(10136)); TEST_ASSERT_EQUAL(BAND_60M, bandFromKhz(5357));
  TEST_ASSERT_EQUAL(BAND_6M, bandFromKhz(50313)); TEST_ASSERT_EQUAL(BAND_VHF, bandFromKhz(144300)); TEST_ASSERT_EQUAL(BAND_UHF, bandFromKhz(432100));
  TEST_ASSERT_EQUAL(BAND_SHF, bandFromKhz(10368000)); TEST_ASSERT_EQUAL(BAND_UNKNOWN, bandFromKhz(500));
  TEST_ASSERT_EQUAL_STRING("40m", bandName(BAND_40M)); }
void test_modes() {
  TEST_ASSERT_EQUAL(MODE_DIGI, modeFrom("FT8 -12dB", 14074)); TEST_ASSERT_EQUAL(MODE_CW, modeFrom("CW 599", 14025));
  TEST_ASSERT_EQUAL(MODE_SSB, modeFrom("", 14250)); TEST_ASSERT_EQUAL(MODE_CW, modeFrom("", 14025)); }
void test_filter() {
  TEST_ASSERT_TRUE(passesFilter(BAND_30M, MODE_CW, 1u << BAND_30M, 0)); TEST_ASSERT_FALSE(passesFilter(BAND_20M, MODE_CW, 1u << BAND_30M, 0));
  TEST_ASSERT_FALSE(passesFilter(BAND_60M, MODE_SSB, 1u << BAND_40M, 0)); TEST_ASSERT_TRUE(passesFilter(BAND_60M, MODE_SSB, 0, 0));
  TEST_ASSERT_FALSE(passesFilter(BAND_20M, MODE_DIGI, 0, 1)); TEST_ASSERT_TRUE(passesFilter(BAND_20M, MODE_DIGI, 0, 4)); }
int main() { UNITY_BEGIN(); RUN_TEST(test_bands); RUN_TEST(test_modes); RUN_TEST(test_filter); return UNITY_END(); }
