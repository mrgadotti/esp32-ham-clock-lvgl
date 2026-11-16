#include <unity.h>
#include <string.h>
#include "aprs_parse.h"
void setUp() {} void tearDown() {}

void test_uncompressed() {
  AprsStation s;
  TEST_ASSERT_TRUE(aprsParseFrame("PY2XX>APRS:!4903.50N/07201.75W-Test", s));
  TEST_ASSERT_EQUAL_STRING("PY2XX", s.call);
  TEST_ASSERT_FLOAT_WITHIN(1e-4, 49.058333, s.lat);
  TEST_ASSERT_FLOAT_WITHIN(1e-4, -72.029167, s.lon);
  TEST_ASSERT_EQUAL_CHAR('/', s.sym[0]); TEST_ASSERT_EQUAL_CHAR('-', s.sym[1]);
  TEST_ASSERT_EQUAL_STRING("Test", s.comment);
  TEST_ASSERT_TRUE(s.hasLatLon); TEST_ASSERT_EQUAL(-1, s.bearing);
}
void test_compressed() {
  AprsStation s;
  TEST_ASSERT_TRUE(aprsParseFrame("PY2XX>APRS:=/5L!!<*e7>7P[", s));
  TEST_ASSERT_FLOAT_WITHIN(1e-3, 49.5, s.lat);
  TEST_ASSERT_FLOAT_WITHIN(1e-3, -72.75, s.lon);
}
// Mic-E, derived by hand from the spec: dest "S32UVT" -> digits 3,3,2,5,6,4 => 33 deg 25.64'; 'U' (>=P) => N,
// 'V' => +100 deg longitude offset, 'T' => West. Info bytes '(' '_' 'f' => 40-28=12 (+100), 95-28=67 => 7 min,
// 102-28=74 => .74 => 112 deg 07.74' W. Symbol: code 'j', table '/'.
// Cross-check: the original parseAprsMicE (L13080, no String dependency) was compiled once on the host from
// the extracted source and produced lat 33.427334 lon -112.129000 sym '/' 'j' for this input (same values).
void test_mic_e() {
  AprsStation s;
  TEST_ASSERT_TRUE(aprsParseFrame("PY2XX>S32UVT:`(_fn\"Oj/", s));
  TEST_ASSERT_FLOAT_WITHIN(1e-3, 33.4273, s.lat);
  TEST_ASSERT_FLOAT_WITHIN(1e-3, -112.129, s.lon);
  TEST_ASSERT_EQUAL_CHAR('/', s.sym[0]); TEST_ASSERT_EQUAL_CHAR('j', s.sym[1]);
}
void test_object() {
  AprsStation s;
  TEST_ASSERT_TRUE(aprsParseFrame("PY2XX>APRS:;LEADER   *092345z4903.50N/07201.75W>Obj 146.520MHz", s));
  TEST_ASSERT_EQUAL_STRING("LEADER", s.call);
  TEST_ASSERT_FLOAT_WITHIN(1e-4, 49.058333, s.lat);
  TEST_ASSERT_EQUAL_STRING("Obj 146.520MHz", s.comment);
  TEST_ASSERT_FLOAT_WITHIN(1e-3, 146.52, s.freqMhz);
}
void test_item_and_wx_symbol() {
  AprsStation s;
  TEST_ASSERT_TRUE(aprsParseFrame("PY2XX>APRS:)ITEM!4903.50N/07201.75W-x", s));
  TEST_ASSERT_EQUAL_STRING("ITEM", s.call);
  TEST_ASSERT_TRUE(aprsParseFrame("W1AW>APRS,WIDE1-1:@092345z4903.50N/07201.75W_220/004t077h50b09900", s));
  TEST_ASSERT_TRUE(s.isWx);
  TEST_ASSERT_EQUAL_STRING("220/004t077h50b09900", s.comment);
}
void test_comment_sanitized() {
  AprsStation s;
  TEST_ASSERT_TRUE(aprsParseFrame("A>B:!4903.50N/07201.75W-  hi\x01\xC3\xA9 there\r\n", s));
  TEST_ASSERT_EQUAL_STRING("hi there", s.comment);
}
void test_aprs_rejects_garbage() {
  AprsStation s;
  TEST_ASSERT_FALSE(aprsParseFrame("X>Y:", s));
  TEST_ASSERT_FALSE(aprsParseFrame(">:", s));
  TEST_ASSERT_FALSE(aprsParseFrame("PY2XX>APRS:!9999.99N/07201.75W-", s));
  TEST_ASSERT_FALSE(aprsParseFrame("no frame here", s));
  TEST_ASSERT_FALSE(aprsParseFrame(nullptr, s));
}
void test_freq() {
  float f = 0;
  TEST_ASSERT_TRUE(aprsExtractFreq("146.520MHz", f)); TEST_ASSERT_FLOAT_WITHIN(1e-3, 146.52, f);
  TEST_ASSERT_TRUE(aprsExtractFreq("QRV on 433.5 now", f)); TEST_ASSERT_FLOAT_WITHIN(1e-3, 433.5, f);
  TEST_ASSERT_FALSE(aprsExtractFreq("no freq 12 here", f));
}
int main() { UNITY_BEGIN(); RUN_TEST(test_uncompressed); RUN_TEST(test_compressed); RUN_TEST(test_mic_e);
  RUN_TEST(test_object); RUN_TEST(test_item_and_wx_symbol); RUN_TEST(test_comment_sanitized);
  RUN_TEST(test_aprs_rejects_garbage); RUN_TEST(test_freq); return UNITY_END(); }
