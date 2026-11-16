#include <unity.h>
#include <string.h>
#include <stdio.h>
#include "alert_rules.h"
void setUp() {} void tearDown() {}

static AprsStation st(const char* call, int16_t dist = -1) {
  AprsStation s = AprsStation(); strcpy(s.call, call); s.distKm = dist; s.hasLatLon = true; return s;
}
static AlertCfg cfg(const char* watch) {
  AlertCfg c = AlertCfg(); strcpy(c.watchList, watch); c.alertEnabled = true; c.minSeconds = 60; c.alertKm = 50; return c;
}

void test_alert_wildcard() {
  TEST_ASSERT_TRUE(alertCallMatches("PY2*", "PY2XX-9"));
  TEST_ASSERT_TRUE(alertCallMatches("py2*", "PY2XX"));
  TEST_ASSERT_FALSE(alertCallMatches("PY2*", "PU2XX"));
  TEST_ASSERT_FALSE(alertCallMatches("*", "PY2XX"));
}
void test_alert_base_matches_ssid() {
  TEST_ASSERT_TRUE(alertCallMatches("PY2XX", "PY2XX"));
  TEST_ASSERT_TRUE(alertCallMatches("PY2XX", "PY2XX-9"));
  TEST_ASSERT_FALSE(alertCallMatches("PY2XX", "PY2XXX"));
  TEST_ASSERT_TRUE(alertCallMatches("PY2XX-9", "PY2XX-9"));
  TEST_ASSERT_FALSE(alertCallMatches("PY2XX-9", "PY2XX-7"));
  TEST_ASSERT_FALSE(alertCallMatches("PY2XX-9", "PY2XX"));
  TEST_ASSERT_FALSE(alertCallMatches("", "PY2XX"));
}
void test_alert_cooldown() {
  AlertRules r; r.configure(cfg("PY2XX, W1AW-5"));
  AprsStation a = st("PY2XX-3");
  TEST_ASSERT_TRUE(r.shouldAlert(a, 1000));
  TEST_ASSERT_FALSE(r.shouldAlert(a, 30000));
  TEST_ASSERT_TRUE(r.shouldAlert(st("W1AW-5"), 30000));  // per-call
  TEST_ASSERT_TRUE(r.shouldAlert(a, 62000));
  TEST_ASSERT_FALSE(r.shouldAlert(st("N0CALL"), 70000));  // not watched
}
void test_alert_mobile_ssid_distance() {
  AlertCfg c = cfg(""); c.rangeEnabled = true;
  AlertRules r; r.configure(c);
  TEST_ASSERT_TRUE(r.shouldAlert(st("PY1AB-9", 10), 0));
  TEST_ASSERT_FALSE(r.shouldAlert(st("PY1AC-9", 80), 0));
  TEST_ASSERT_FALSE(r.shouldAlert(st("PY1AD-5", 10), 0));
  TEST_ASSERT_FALSE(r.shouldAlert(st("PY1AE-7", -1), 0));  // distance unknown
  TEST_ASSERT_TRUE(r.shouldAlert(st("PY1AF-7", 50), 0));
}
void test_alert_wx_and_disabled() {
  AlertCfg c = cfg(""); c.wxEnabled = true;
  AlertRules r; r.configure(c);
  AprsStation w = st("WX1"); w.isWx = true; strcpy(w.comment, "220/004t077h50b09900");
  TEST_ASSERT_TRUE(r.shouldAlert(w, 0));
  AprsStation n = st("WX2"); n.isWx = true; strcpy(n.comment, "220/004");
  TEST_ASSERT_FALSE(r.shouldAlert(n, 0));  // needs temp + humidity
  c.alertEnabled = false; r.configure(c);
  TEST_ASSERT_FALSE(r.shouldAlert(w, 0));
}
void test_alert_tokenizer_separators() {
  AlertRules r; r.configure(cfg("PY2XX PY1AB;PU2*"));
  TEST_ASSERT_TRUE(r.shouldAlert(st("PY2XX"), 0));
  TEST_ASSERT_TRUE(r.shouldAlert(st("PY1AB"), 0));
  TEST_ASSERT_TRUE(r.shouldAlert(st("PU2ZZ"), 0));
  TEST_ASSERT_FALSE(r.shouldAlert(st("PY3ZZ"), 0));
}
void test_alert_eviction_prefers_expired() {
  AlertRules r; r.configure(cfg("C*"));
  char name[8];
  TEST_ASSERT_TRUE(r.shouldAlert(st("C00"), 0));  // will be the expired one
  for (int i = 1; i < 16; i++) { sprintf(name, "C%02d", i); TEST_ASSERT_TRUE(r.shouldAlert(st(name), 50000 + i)); }
  TEST_ASSERT_TRUE(r.shouldAlert(st("C99"), 61000));   // table full: evicts C00 (expired)
  TEST_ASSERT_FALSE(r.shouldAlert(st("C01"), 61000));  // still cooling, must not have been evicted
  TEST_ASSERT_FALSE(r.shouldAlert(st("C15"), 61000));
}
int main() { UNITY_BEGIN(); RUN_TEST(test_alert_wildcard); RUN_TEST(test_alert_base_matches_ssid);
  RUN_TEST(test_alert_cooldown); RUN_TEST(test_alert_mobile_ssid_distance); RUN_TEST(test_alert_wx_and_disabled);
  RUN_TEST(test_alert_tokenizer_separators); RUN_TEST(test_alert_eviction_prefers_expired); return UNITY_END(); }
