#include <unity.h>
#include <string.h>
#include "weather_alerts.h"
void setUp() {} void tearDown() {}

static WeatherData base() { WeatherData w; memset(&w, 0, sizeof w); return w; }
static const WxAlertCfg CFG = {true, true, 15.0f};

void test_alert_storm_now() {
  WeatherAlertScheduler s; WeatherAlertInfo o; WeatherData w = base();
  w.valid = true; w.weatherId = 211; w.fetchedAt = 1000;
  TEST_ASSERT_TRUE(s.due(w, 60000, CFG, o));
  TEST_ASSERT_TRUE(o.stormPossible); TEST_ASSERT_TRUE(o.stormNow); TEST_ASSERT_EQUAL_UINT8(1, o.stormEtaHours);
  WeatherAlertScheduler s2;  // data older than 45 min
  TEST_ASSERT_FALSE(s2.due(w, 1000 + 46UL * 60000UL, CFG, o));
  WxAlertCfg off = {false, true, 15.0f};
  TEST_ASSERT_FALSE(s.due(w, 5UL * 60000UL, off, o));
}

void test_alert_wind_gust() {
  WeatherAlertScheduler s; WeatherAlertInfo o; WeatherData w = base();
  w.forecastFetchedAt = 1; w.fcWindMaxMs = 10; w.fcWindGustMs = 19.9f;
  TEST_ASSERT_FALSE(s.due(w, 100, CFG, o));
  w.fcWindGustMs = 20.0f; w.windMs = 12;
  TEST_ASSERT_TRUE(s.due(w, 200, CFG, o));
  TEST_ASSERT_TRUE(o.windPossible); TEST_ASSERT_FALSE(o.stormPossible);
  TEST_ASSERT_EQUAL_FLOAT(12, o.maxWindMs); TEST_ASSERT_EQUAL_FLOAT(20, o.maxWindGustMs);
  WeatherAlertScheduler s2; w.fcWindGustMs = 0; w.fcWindMaxMs = 15;  // sustained wind at threshold
  TEST_ASSERT_TRUE(s2.due(w, 200, CFG, o));
}

void test_alert_repeat_minutes() {
  WeatherAlertScheduler s; WeatherAlertInfo o; WeatherData w = base();
  w.forecastFetchedAt = 1; w.stormEtaMin = 150;  // 3 h -> repeat every 3 min
  TEST_ASSERT_TRUE(s.due(w, 10, CFG, o));
  TEST_ASSERT_TRUE(o.stormPossible); TEST_ASSERT_FALSE(o.stormNow); TEST_ASSERT_EQUAL_UINT8(3, o.stormEtaHours);
  TEST_ASSERT_FALSE(s.due(w, 10 + 2 * 60000UL, CFG, o));
  TEST_ASSERT_TRUE(s.due(w, 10 + 3 * 60000UL, CFG, o));
  // ETA shrinks with elapsed time: 150 - 100 = 50 min -> 1 min repeat
  TEST_ASSERT_TRUE(s.due(w, 1 + 100UL * 60000UL, CFG, o));
  TEST_ASSERT_EQUAL_UINT8(1, o.stormEtaHours);
  // clamp at 6
  WeatherAlertScheduler s2; w.stormEtaMin = 360; w.forecastFetchedAt = 1;
  TEST_ASSERT_TRUE(s2.due(w, 5, CFG, o)); TEST_ASSERT_EQUAL_UINT8(6, o.stormEtaHours);
}

void test_alert_millis_wrap() {
  WeatherAlertScheduler s; WeatherAlertInfo o; WeatherData w = base();
  uint32_t t = 0xFFFFFFFFu - 30000u; w.forecastFetchedAt = t; w.stormEtaMin = 30;
  TEST_ASSERT_TRUE(s.due(w, t, CFG, o));
  TEST_ASSERT_FALSE(s.due(w, t + 30000u, CFG, o));
  TEST_ASSERT_TRUE(s.due(w, t + 60000u, CFG, o));  // wrapped past 0
}

void test_storm_age_boundary() {
  WeatherData w = base(); WeatherAlertInfo o;
  w.valid = true; w.weatherId = 200; w.fetchedAt = 1000;
  WeatherAlertScheduler a, b;
  TEST_ASSERT_TRUE(a.due(w, 1000 + 45UL * 60000UL, CFG, o));
  TEST_ASSERT_FALSE(b.due(w, 1000 + 45UL * 60000UL + 1, CFG, o));
}

void test_forecast_stamp_in_future() {
  WeatherData w = base(); WeatherAlertInfo o; WeatherAlertScheduler s;
  w.forecastFetchedAt = 5000; w.stormEtaMin = 150;  // now < stamp: 0 elapsed, eta stays 150 -> 3
  TEST_ASSERT_TRUE(s.due(w, 1000, CFG, o)); TEST_ASSERT_EQUAL_UINT8(3, o.stormEtaHours);
}

void test_storm_data_40min_old_still_alerts() {
  WeatherData w = base(); WeatherAlertInfo o; WeatherAlertScheduler s;
  w.valid = true; w.weatherId = 200; w.fetchedAt = 1000;
  TEST_ASSERT_TRUE(s.due(w, 1000 + 40UL * 60000UL, CFG, o)); TEST_ASSERT_TRUE(o.stormNow);
}

void test_storm_data_50min_old_does_not_alert() {
  WeatherData w = base(); WeatherAlertInfo o; WeatherAlertScheduler s;
  w.valid = true; w.weatherId = 200; w.fetchedAt = 1000;
  TEST_ASSERT_FALSE(s.due(w, 1000 + 50UL * 60000UL, CFG, o));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_storm_age_boundary); RUN_TEST(test_forecast_stamp_in_future);
  RUN_TEST(test_alert_storm_now); RUN_TEST(test_alert_wind_gust);
  RUN_TEST(test_storm_data_40min_old_still_alerts); RUN_TEST(test_storm_data_50min_old_does_not_alert);
  RUN_TEST(test_alert_repeat_minutes); RUN_TEST(test_alert_millis_wrap);
  return UNITY_END();
}
