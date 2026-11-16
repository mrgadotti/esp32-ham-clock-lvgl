#include <unity.h>
#include <string.h>
#include "dx_parse.h"
#include "band_mode.h"
void setUp() {} void tearDown() {}
static const uint32_t DAY = 86400u * 19000;
void test_parse_basic() {
  Spot s;
  TEST_ASSERT_TRUE(parseDxLine("DX de PY2XX:     14025.0  K1ABC        CW 599                         1234Z", DAY + 13 * 3600, s));
  TEST_ASSERT_EQUAL_STRING("K1ABC", s.call); TEST_ASSERT_EQUAL_STRING("PY2XX", s.spotter);
  TEST_ASSERT_EQUAL_UINT32(14025, s.khz); TEST_ASSERT_EQUAL_STRING("CW 599", s.comment);
  TEST_ASSERT_EQUAL(B20, s.band); TEST_ASSERT_EQUAL(MODE_CW, s.mode);
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
  TEST_ASSERT_FALSE(parseDxLine("hello", DAY, s)); TEST_ASSERT_FALSE(parseDxLine("DX de A1B: abc K1ABC cw 1234Z", DAY, s));
}
void test_long_comment_truncated_and_sanitized() {
  char l[400] = "DX de A1B: 14025.0 K1ABC ";
  size_t n = strlen(l);
  for (int i = 0; i < 200; i++) l[n++] = i == 5 ? 0x01 : i == 7 ? (char)0xC3 : 'x';
  l[n] = 0;
  Spot s; TEST_ASSERT_TRUE(parseDxLine(l, DAY, s));
  size_t c = strlen(s.comment); TEST_ASSERT_TRUE(c > 0 && c <= 39);
  for (size_t i = 0; i < c; i++) TEST_ASSERT_TRUE(s.comment[i] >= 0x20 && s.comment[i] < 0x7F);
}
int main() { UNITY_BEGIN(); RUN_TEST(test_parse_basic); RUN_TEST(test_parse_previous_day); RUN_TEST(test_parse_locator_in_comment);
  RUN_TEST(test_dx_rejects_garbage); RUN_TEST(test_long_comment_truncated_and_sanitized); return UNITY_END(); }
