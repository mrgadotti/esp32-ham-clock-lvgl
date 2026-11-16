#include <unity.h>
#include <string.h>
#include <ArduinoJson.h>
#include "pota_parse.h"
#include "band_mode.h"
void setUp() {} void tearDown() {}

static const char* FIX =
  "[{\"activator\":\"K1ABC\",\"frequency\":\"14074\",\"mode\":\"ft8\",\"reference\":\"US-0001\","
  "\"spotTime\":\"2026-11-16T12:34:56\",\"locationDesc\":\"US-MA\",\"comments\":\"hi\"},"
  "{\"activator\":\"W2XYZ\",\"frequency\":\"7032.5\",\"mode\":\"CW\",\"reference\":\"US-0002\","
  "\"spotTime\":\"2026-11-16T12:40:00\",\"locationDesc\":\"US-NY\"}]";

void test_iso_to_unix() {
  uint32_t t = 0;
  TEST_ASSERT_TRUE(isoUtcToUnix("2026-11-16T12:34:56", t));
  TEST_ASSERT_EQUAL_UINT32(1794832496u, t);
  TEST_ASSERT_TRUE(isoUtcToUnix("2024-02-29T00:00:00", t));
  TEST_ASSERT_EQUAL_UINT32(1709164800u, t);
  TEST_ASSERT_FALSE(isoUtcToUnix("2026-13-16T12:34:56", t));
  TEST_ASSERT_FALSE(isoUtcToUnix("garbage", t));
}
void test_pota_fixture() {
  JsonDocument doc; TEST_ASSERT_TRUE(!deserializeJson(doc, FIX));
  Spot a, b;
  TEST_ASSERT_TRUE(potaFromJson(doc[0].as<JsonObjectConst>(), a));
  TEST_ASSERT_TRUE(potaFromJson(doc[1].as<JsonObjectConst>(), b));
  TEST_ASSERT_EQUAL_STRING("K1ABC", a.call); TEST_ASSERT_EQUAL_UINT32(14074, a.khz);
  TEST_ASSERT_EQUAL(MODE_DIGI, a.mode); TEST_ASSERT_EQUAL(B20, a.band);
  TEST_ASSERT_EQUAL_UINT32(1794832496u, a.utc);
  TEST_ASSERT_EQUAL_STRING("US-MA", a.country);
  TEST_ASSERT_EQUAL_STRING("US-0001 US-MA", a.comment);
  TEST_ASSERT_EQUAL_STRING("W2XYZ", b.call); TEST_ASSERT_EQUAL_UINT32(7033, b.khz);
  TEST_ASSERT_EQUAL(MODE_CW, b.mode); TEST_ASSERT_EQUAL(B40, b.band);
}
void test_pota_rejects_garbage() {
  JsonDocument doc; deserializeJson(doc, "{\"frequency\":\"14074\",\"reference\":\"X\"}");
  Spot s; memset(&s, 0x5A, sizeof s); Spot ref = s;
  TEST_ASSERT_FALSE(potaFromJson(doc.as<JsonObjectConst>(), s));
  TEST_ASSERT_EQUAL_MEMORY(&ref, &s, sizeof s);
  deserializeJson(doc, "{\"activator\":\"K1ABC\",\"frequency\":\"abc\"}");
  TEST_ASSERT_FALSE(potaFromJson(doc.as<JsonObjectConst>(), s));
}
void test_pota_sort() {
  Spot a[3] = {}; a[0].utc = 5; a[1].utc = 9; a[2].utc = 7;
  potaSortByTimeDesc(a, 3);
  TEST_ASSERT_EQUAL_UINT32(9, a[0].utc); TEST_ASSERT_EQUAL_UINT32(7, a[1].utc); TEST_ASSERT_EQUAL_UINT32(5, a[2].utc);
}
int main() {
  UNITY_BEGIN(); RUN_TEST(test_iso_to_unix); RUN_TEST(test_pota_fixture);
  RUN_TEST(test_pota_rejects_garbage); RUN_TEST(test_pota_sort); return UNITY_END();
}
