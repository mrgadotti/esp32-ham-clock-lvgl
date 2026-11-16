#include <stdio.h>
#include <string.h>
#include <unity.h>
#include "qrz_cache.h"
void setUp() {} void tearDown() {}

static const char* OK_XML = "<QRZDatabase><Session><Key>abc123</Key></Session><Callsign><call>W1AW</call>"
  "<grid>FN31pr</grid><country>United States</country><lat>41.7</lat><lon>-72.7</lon></Callsign></QRZDatabase>";

void test_cache_ttl() {
  QrzCache c; QrzInfo i = {}, o = {};
  strcpy(i.country, "Germany");
  c.put("dl1abc", i, 1000);
  TEST_ASSERT_TRUE(c.get("DL1ABC", 1000 + QRZ_CACHE_TTL_MS - 1, o));
  TEST_ASSERT_EQUAL_STRING("Germany", o.country);
  TEST_ASSERT_FALSE(c.get("DL1ABC", 1000 + QRZ_CACHE_TTL_MS, o));
  c.put("X1", i, 0xFFFFFF00u);  // wrap-safe
  TEST_ASSERT_TRUE(c.get("X1", 0x100u, o));
}
void test_cache_evicts_oldest() {
  QrzCache c; QrzInfo i = {}, o;
  char call[8];
  for (int k = 0; k < QRZ_CACHE_SIZE + 1; k++) { snprintf(call, sizeof call, "A%d", k); c.put(call, i, 1000 + k); }
  TEST_ASSERT_FALSE(c.get("A0", 2000, o));
  TEST_ASSERT_TRUE(c.get("A1", 2000, o));
  TEST_ASSERT_TRUE(c.get("A20", 2000, o));
}
void test_queue_dedupe() {
  QrzQueue q;
  TEST_ASSERT_TRUE(q.enqueue("w1aw", 0));
  TEST_ASSERT_FALSE(q.enqueue("W1AW", 0));
  TEST_ASSERT_FALSE(q.enqueue("!!", 0));
  TEST_ASSERT_TRUE(q.contains("w1aw"));
  TEST_ASSERT_EQUAL(1, q.size());
  for (int k = 0; k < 30; k++) { char c[8]; snprintf(c, sizeof c, "B%d", k); q.enqueue(c, 0); }
  TEST_ASSERT_EQUAL(QRZ_QUEUE_SIZE, q.size());
}
void test_queue_interval_retry() {
  QrzQueue q; char c[QRZ_CALL_LEN];
  q.enqueue("AA1", 100); q.enqueue("BB2", 100);
  TEST_ASSERT_TRUE(q.next(100, c)); TEST_ASSERT_EQUAL_STRING("AA1", c);
  TEST_ASSERT_FALSE(q.next(3099, c));
  q.fail("AA1", 100);  // retry at 5100
  TEST_ASSERT_TRUE(q.next(3100, c)); TEST_ASSERT_EQUAL_STRING("BB2", c);
  q.done("BB2");
  TEST_ASSERT_FALSE(q.next(5099, c));  // AA1 not due yet
  TEST_ASSERT_TRUE(q.next(6100, c)); TEST_ASSERT_EQUAL_STRING("AA1", c);
}
void test_queue_drop_after_two() {
  QrzQueue q;
  q.enqueue("AA1", 0);
  q.fail("AA1", 0); TEST_ASSERT_EQUAL(1, q.size());
  q.fail("AA1", 5000); TEST_ASSERT_EQUAL(0, q.size());
}
void test_parse_xml() {
  QrzInfo o = {};
  TEST_ASSERT_TRUE(qrzParseXml(OK_XML, o));
  TEST_ASSERT_EQUAL_STRING("FN31pr", o.grid);
  TEST_ASSERT_EQUAL_STRING("United States", o.country);
  TEST_ASSERT_TRUE(o.hasLatLon);
  TEST_ASSERT_FLOAT_WITHIN(0.01, 41.7, o.lat);
  TEST_ASSERT_FLOAT_WITHIN(0.01, -72.7, o.lon);
}
void test_parse_session_error() {
  const char* e = "<QRZDatabase><Session><Error>Session Timeout</Error></Session></QRZDatabase>";
  QrzInfo o = {}; strcpy(o.grid, "KEEP");
  TEST_ASSERT_FALSE(qrzParseXml(e, o));
  TEST_ASSERT_FALSE(qrzParseXml("garbage", o));
  TEST_ASSERT_FALSE(qrzParseXml(nullptr, o));
  TEST_ASSERT_EQUAL_STRING("KEEP", o.grid);
  char k[16] = "keep";
  TEST_ASSERT_FALSE(qrzParseSessionKey(e, k, sizeof k));
  TEST_ASSERT_EQUAL_STRING("keep", k);
}
void test_parse_session_key() {
  char k[16];
  TEST_ASSERT_TRUE(qrzParseSessionKey(OK_XML, k, sizeof k));
  TEST_ASSERT_EQUAL_STRING("abc123", k);
  char tiny[3];
  TEST_ASSERT_FALSE(qrzParseSessionKey(OK_XML, tiny, sizeof tiny));
}
void test_parse_entities_and_sanitize() {
  QrzInfo o = {};
  TEST_ASSERT_TRUE(qrzParseXml("<grid>FN31</grid><country>Trinidad &amp; Tobago &quot;x&quot;</country>", o));
  TEST_ASSERT_EQUAL_STRING("Trinidad & Tobago \"x\"", o.country);
  TEST_ASSERT_TRUE(qrzParseXml("<grid>FN31</grid><country>C\xC3\xB4te</country>", o));
  TEST_ASSERT_EQUAL_STRING("C??te", o.country);
  QrzInfo p = {}; strcpy(p.grid, "KEEP");
  TEST_ASSERT_FALSE(qrzParseXml("<grid>FN<b>31</b></grid>", p));
  TEST_ASSERT_FALSE(qrzParseXml("<grid>&lt;b&gt;</grid>", p));
  TEST_ASSERT_EQUAL_STRING("KEEP", p.grid);
}
void test_parse_dxcc_fallback() {
  QrzInfo o = {};
  TEST_ASSERT_TRUE(qrzParseXml("<dxcc>Germany</dxcc>", o));
  TEST_ASSERT_EQUAL_STRING("Germany", o.country);
  TEST_ASSERT_FALSE(o.hasLatLon);
}
void test_parse_bad_coords() {
  QrzInfo o;
  const char* bad[] = {"<lat>abc</lat><lon>1</lon>", "<lat>41.7x</lat><lon>1</lon>", "<lat>91</lat><lon>1</lon>",
                       "<lat>1</lat><lon>-180.5</lon>", "<lat>nan</lat><lon>1</lon>"};
  for (const char* b : bad) {
    char xml[96]; snprintf(xml, sizeof xml, "<grid>FN31</grid>%s", b);
    o = {};
    TEST_ASSERT_TRUE(qrzParseXml(xml, o));
    TEST_ASSERT_FALSE(o.hasLatLon);
  }
  o = {};
  TEST_ASSERT_TRUE(qrzParseXml("<grid>FN31</grid><lat>-90</lat><lon>180</lon>", o));
  TEST_ASSERT_TRUE(o.hasLatLon);
}
void test_queue_wrap() {
  QrzQueue q; char c[QRZ_CALL_LEN];
  uint32_t t = 0xFFFFFF00u;
  q.enqueue("AA1", t);
  TEST_ASSERT_TRUE(q.next(t, c));
  q.fail("AA1", t);                           // due at t+5000 (wrapped)
  TEST_ASSERT_FALSE(q.next(t + 4999, c));
  TEST_ASSERT_TRUE(q.next(t + 5000, c));
  TEST_ASSERT_EQUAL_STRING("AA1", c);
}
void test_normalize() {
  char c[QRZ_CALL_LEN];
  TEST_ASSERT_TRUE(qrzNormalizeCall(" w1aw/p;x ", c)); TEST_ASSERT_EQUAL_STRING("W1AW/PX", c);
  TEST_ASSERT_FALSE(qrzNormalizeCall("", c));
}
int main() {
  UNITY_BEGIN();
  RUN_TEST(test_cache_ttl); RUN_TEST(test_cache_evicts_oldest); RUN_TEST(test_queue_dedupe);
  RUN_TEST(test_queue_interval_retry); RUN_TEST(test_queue_drop_after_two); RUN_TEST(test_parse_xml);
  RUN_TEST(test_parse_session_error); RUN_TEST(test_parse_session_key); RUN_TEST(test_normalize);
  RUN_TEST(test_parse_entities_and_sanitize); RUN_TEST(test_parse_dxcc_fallback);
  RUN_TEST(test_parse_bad_coords); RUN_TEST(test_queue_wrap);
  return UNITY_END();
}
