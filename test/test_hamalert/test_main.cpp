#include <unity.h>
#include <string.h>
#include "hamalert_parse.h"
#include "band_mode.h"
void setUp() {} void tearDown() {}
static const uint32_t NOW = 1794832496u;  // 2026-11-16T12:34:56Z

void test_hamalert_json_line() {
  Spot s;
  TEST_ASSERT_TRUE(hamalertParseLine("{\"dx\":\"DL1ABC\",\"de\":\"K1XX\",\"frequency\":14025.0,\"comment\":\"CW 599\",\"country\":\"Germany\"}", NOW, s));
  TEST_ASSERT_EQUAL_STRING("DL1ABC", s.call); TEST_ASSERT_EQUAL_STRING("K1XX", s.spotter);
  TEST_ASSERT_EQUAL_UINT32(14025, s.khz); TEST_ASSERT_EQUAL(BAND_20M, s.band); TEST_ASSERT_EQUAL(MODE_CW, s.mode);
  TEST_ASSERT_EQUAL_STRING("Germany", s.country); TEST_ASSERT_EQUAL_UINT32(NOW, s.utc);
  TEST_ASSERT_TRUE(hamalertParseLine("{\"call\":\"DL1ABC\",\"freq\":\"7.030\",\"mode\":\"ssb\",\"time\":\"2026-11-16T12:00:00\"}", NOW, s));
  TEST_ASSERT_EQUAL_UINT32(7030, s.khz); TEST_ASSERT_EQUAL(MODE_SSB, s.mode); TEST_ASSERT_EQUAL_UINT32(1794830400u, s.utc);
}
void test_hamalert_dx_line() {
  Spot s;
  TEST_ASSERT_TRUE(hamalertParseLine("DX de PY2XX:     14025.0  K1ABC        CW 599                         1234Z\r\n", NOW, s));
  TEST_ASSERT_EQUAL_STRING("K1ABC", s.call); TEST_ASSERT_EQUAL_UINT32(14025, s.khz);
}
void test_hamalert_rejects_garbage() {
  Spot s; memset(&s, 0x5A, sizeof s); Spot ref = s;
  const char* bad[] = {nullptr, "", ">", "Welcome to HamAlert", "{not json", "{\"dx\":\"A1B\"}", "{\"frequency\":14025}", "hello world this is not a spot"};
  for (const char* b : bad) TEST_ASSERT_FALSE(hamalertParseLine(b, NOW, s));
  TEST_ASSERT_EQUAL_MEMORY(&ref, &s, sizeof s);
}
int main() {
  UNITY_BEGIN(); RUN_TEST(test_hamalert_json_line); RUN_TEST(test_hamalert_dx_line);
  RUN_TEST(test_hamalert_rejects_garbage); return UNITY_END();
}
