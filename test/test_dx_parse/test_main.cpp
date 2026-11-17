#include <unity.h>
#include <string.h>
#include <stdio.h>
#include <initializer_list>
#include "dx_parse.h"
#include "band_mode.h"
void setUp() {} void tearDown() {}
static const uint32_t DAY = 86400u * 19000;
void test_parse_basic() {
  Spot s;
  TEST_ASSERT_TRUE(parseDxLine("DX de PY2XX:     14025.0  K1ABC        CW 599                         1234Z", DAY + 13 * 3600, s));
  TEST_ASSERT_EQUAL_STRING("K1ABC", s.call);
  TEST_ASSERT_EQUAL_UINT32(14025, s.khz); TEST_ASSERT_EQUAL_STRING("CW 599", s.comment);
  TEST_ASSERT_EQUAL(BAND_20M, s.band); TEST_ASSERT_EQUAL(MODE_CW, s.mode);
  TEST_ASSERT_EQUAL_UINT32(DAY + 12 * 3600 + 34 * 60, s.utc);
  TEST_ASSERT_EQUAL(-1, s.distKm); TEST_ASSERT_FALSE(s.hasLatLon);
}
void test_parse_previous_day() {
  Spot s;
  TEST_ASSERT_TRUE(parseDxLine("DX de A1B: 7010.0 K1ABC CW 2350Z", DAY + 10 * 60, s));
  TEST_ASSERT_EQUAL_UINT32(DAY - 86400 + 23 * 3600 + 50 * 60, s.utc);
}
void test_parse_locator_in_comment() {
  Spot s; TEST_ASSERT_TRUE(parseDxLine("DX de PY2XX: 14025.0 K1ABC CW FN31 599 1234Z", DAY + 13 * 3600, s));
  TEST_ASSERT_EQUAL_STRING("FN31", s.loc);
  TEST_ASSERT_TRUE(parseDxLine("DX de PY2XX: 14025.0 K1ABC jo82lk 1234Z", DAY + 13 * 3600, s));
  TEST_ASSERT_EQUAL_STRING("JO82LK", s.loc);
}
void test_dx_rejects_garbage() {
  Spot s;
  TEST_ASSERT_FALSE(parseDxLine("", DAY, s)); TEST_ASSERT_FALSE(parseDxLine("DX de", DAY, s));
  TEST_ASSERT_FALSE(parseDxLine("hello", DAY, s));
  for (const char* bad : {"nan", "inf", "1e12", "0x10", "-5"}) {
    char l[64]; snprintf(l, sizeof l, "DX de A1B: %s K1ABC cw 1234Z", bad);
    TEST_ASSERT_FALSE(parseDxLine(l, DAY, s));
  }
  Spot k{}; k.khz = 77; strcpy(k.call, "KEEP");
  TEST_ASSERT_FALSE(parseDxLine("DX de A1B: abc K1ABC cw 1234Z", DAY, k));
  TEST_ASSERT_EQUAL_UINT32(77, k.khz); TEST_ASSERT_EQUAL_STRING("KEEP", k.call); TEST_ASSERT_FALSE(parseDxLine("DX de A1B: abc K1ABC cw 1234Z", DAY, s));
}
void test_long_comment_truncated_and_sanitized() {
  char l[400] = "DX de A1B: 14025.0 K1ABC ";
  size_t n = strlen(l);
  for (int i = 0; i < 200; i++) l[n++] = i == 5 ? 0x01 : i == 7 ? (char)0xC3 : 'x';
  l[n] = 0;
  Spot s; TEST_ASSERT_TRUE(parseDxLine(l, DAY, s));
  size_t c = strlen(s.comment); TEST_ASSERT_EQUAL(39, c);
  TEST_ASSERT_EQUAL('?', s.comment[5]); TEST_ASSERT_EQUAL('?', s.comment[7]);
  for (size_t i = 0; i < c; i++) TEST_ASSERT_TRUE(s.comment[i] >= 0x20 && s.comment[i] < 0x7F);
}
void test_freq_rounding_and_no_locator() {
  Spot s;
  TEST_ASSERT_TRUE(parseDxLine("DX de A1B: 7074.5 K1ABC cw 1234Z", DAY, s)); TEST_ASSERT_EQUAL_UINT32(7075, s.khz);  // rounds to nearest
  TEST_ASSERT_TRUE(parseDxLine("DX de A1B: 144300.0 K1ABC cw 1234Z", DAY, s)); TEST_ASSERT_EQUAL_UINT32(144300, s.khz);
  for (const char* c : {"CW 599", "FT8 -12", "K1AB", "RR73"}) {
    char l[96]; snprintf(l, sizeof l, "DX de A1B: 14025.0 K1ABC %s 1234Z", c);
    TEST_ASSERT_TRUE(parseDxLine(l, DAY, s)); TEST_ASSERT_EQUAL_STRING("", s.loc);
  }
  TEST_ASSERT_TRUE(parseDxLine("DX de A1B: 14025.0 K1ABC cw 1234Z\r\n", DAY + 13 * 3600, s));
  TEST_ASSERT_EQUAL_UINT32(DAY + 12 * 3600 + 34 * 60, s.utc); TEST_ASSERT_EQUAL_STRING("cw", s.comment);
}
int main() { UNITY_BEGIN(); RUN_TEST(test_parse_basic); RUN_TEST(test_parse_previous_day); RUN_TEST(test_parse_locator_in_comment);
  RUN_TEST(test_dx_rejects_garbage); RUN_TEST(test_freq_rounding_and_no_locator); RUN_TEST(test_long_comment_truncated_and_sanitized); return UNITY_END(); }
