#include <unity.h>
#include <string.h>
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
int main() { UNITY_BEGIN(); RUN_TEST(test_alert_wildcard); RUN_TEST(test_alert_base_matches_ssid);
  RUN_TEST(test_alert_cooldown); RUN_TEST(test_alert_mobile_ssid_distance); RUN_TEST(test_alert_wx_and_disabled); return UNITY_END(); }
