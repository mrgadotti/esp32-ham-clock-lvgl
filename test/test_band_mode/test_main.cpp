#include <unity.h>
#include "band_mode.h"
void setUp() {} void tearDown() {}
void test_bands() {
  TEST_ASSERT_EQUAL(B40, bandFromKhz(7074)); TEST_ASSERT_EQUAL(B30, bandFromKhz(10136)); TEST_ASSERT_EQUAL(B60, bandFromKhz(5357));
  TEST_ASSERT_EQUAL(B6, bandFromKhz(50313)); TEST_ASSERT_EQUAL(VHF, bandFromKhz(144300)); TEST_ASSERT_EQUAL(UHF, bandFromKhz(432100));
  TEST_ASSERT_EQUAL(SHF, bandFromKhz(10368000)); TEST_ASSERT_EQUAL(BAND_UNKNOWN, bandFromKhz(500));
  TEST_ASSERT_EQUAL_STRING("40m", bandName(B40)); }
void test_modes() {
  TEST_ASSERT_EQUAL(MODE_DIGI, modeFrom("FT8 -12dB", 14074)); TEST_ASSERT_EQUAL(MODE_CW, modeFrom("CW 599", 14025));
  TEST_ASSERT_EQUAL(MODE_SSB, modeFrom("", 14250)); TEST_ASSERT_EQUAL(MODE_CW, modeFrom("", 14025)); }
void test_filter() {
  TEST_ASSERT_TRUE(passesFilter(B30, MODE_CW, 1u << B30, 0)); TEST_ASSERT_FALSE(passesFilter(B20, MODE_CW, 1u << B30, 0));
  TEST_ASSERT_FALSE(passesFilter(B60, MODE_SSB, 1u << B40, 0)); TEST_ASSERT_TRUE(passesFilter(B60, MODE_SSB, 0, 0));
  TEST_ASSERT_FALSE(passesFilter(B20, MODE_DIGI, 0, 1)); TEST_ASSERT_TRUE(passesFilter(B20, MODE_DIGI, 0, 4)); }
int main() { UNITY_BEGIN(); RUN_TEST(test_bands); RUN_TEST(test_modes); RUN_TEST(test_filter); return UNITY_END(); }
