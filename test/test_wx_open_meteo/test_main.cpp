#include <unity.h>
#include <string.h>
#include "wx_open_meteo.h"
void setUp() {} void tearDown() {}

static const char* HOURLY7 =
  "\"hourly\":{\"time\":[],\"temperature_2m\":[20,21,22,23.5,24,25,26.5],\"relative_humidity_2m\":[70,71,72,73,74,75,76],"
  "\"weather_code\":[0,0,1,61,2,3,95],\"wind_speed_10m\":[1,2,3,4.5,5,6,7.5],\"wind_gusts_10m\":[2,3,4,5,6,7,11.2]}";
static const char* DAILY3 =
  "\"daily\":{\"time\":[],\"weather_code\":[95,61,71],\"temperature_2m_max\":[29.9,29.8,32.4],"
  "\"temperature_2m_min\":[17.8,19.1,19.8],\"relative_humidity_2m_mean\":[80,66.4,70],\"wind_speed_10m_max\":[3,4.2,5]}";

static bool parse(const char* json, WeatherData& w) {
  JsonDocument d; deserializeJson(d, json);
  return openMeteoParse(d, w);
}
static void sample(char* buf, size_t cap, int isDay, const char* hourly) {
  snprintf(buf, cap, "{\"current\":{\"temperature_2m\":22.8,\"relative_humidity_2m\":74,\"surface_pressure\":929.7,"
    "\"wind_speed_10m\":1.3,\"weather_code\":2,\"is_day\":%d},%s,%s}", isDay, hourly, DAILY3);
}

void test_wmo_known_codes() {
  TEST_ASSERT_EQUAL_INT(800, wmoToOwmId(0)); TEST_ASSERT_EQUAL_INT(804, wmoToOwmId(3));
  TEST_ASSERT_EQUAL_INT(500, wmoToOwmId(61)); TEST_ASSERT_EQUAL_INT(211, wmoToOwmId(95));
  TEST_ASSERT_EQUAL_INT(600, wmoToOwmId(71)); TEST_ASSERT_EQUAL_INT(741, wmoToOwmId(48));
  TEST_ASSERT_EQUAL_INT(511, wmoToOwmId(67)); TEST_ASSERT_EQUAL_INT(202, wmoToOwmId(99));
}
void test_wmo_unknown_is_clear() { TEST_ASSERT_EQUAL_INT(800, wmoToOwmId(100)); }

void test_parse_sample() {
  char b[1200]; sample(b, sizeof b, 1, HOURLY7);
  WeatherData w; memset(&w, 0, sizeof w);
  TEST_ASSERT_TRUE(parse(b, w));
  TEST_ASSERT_EQUAL_FLOAT(22.8f, w.tempC); TEST_ASSERT_EQUAL_INT(74, w.humidity);
  TEST_ASSERT_EQUAL_INT(930, w.pressure); TEST_ASSERT_EQUAL_FLOAT(1.3f, w.windMs);
  TEST_ASSERT_EQUAL_INT(802, w.weatherId); TEST_ASSERT_EQUAL_STRING("01d", w.iconCode);
  for (int i = 0; i < 4; i++) TEST_ASSERT_TRUE(w.detailValid[i]);
  TEST_ASSERT_FALSE(w.detailValid[4]);
  TEST_ASSERT_EQUAL_FLOAT(23.5f, w.detailTempC[0]); TEST_ASSERT_EQUAL_INT(500, w.detailWeatherId[0]);
  TEST_ASSERT_EQUAL_FLOAT(26.5f, w.detailTempC[1]); TEST_ASSERT_EQUAL_INT(211, w.detailWeatherId[1]);
  TEST_ASSERT_EQUAL_FLOAT(29.8f, w.detailTempC[2]); TEST_ASSERT_EQUAL_INT(66, w.detailHumidity[2]);
  TEST_ASSERT_EQUAL_FLOAT(5.0f, w.detailWindMs[3]); TEST_ASSERT_EQUAL_INT(600, w.detailWeatherId[3]);
  TEST_ASSERT_EQUAL_STRING("01d", w.detailIconCode[2]);
  TEST_ASSERT_TRUE(w.nightTempValid[0]); TEST_ASSERT_TRUE(w.nightTempValid[1]);
  TEST_ASSERT_EQUAL_FLOAT(19.1f, w.nightTempC[0]); TEST_ASSERT_EQUAL_FLOAT(19.8f, w.nightTempC[1]);
  TEST_ASSERT_EQUAL_FLOAT(7.5f, w.fcWindMaxMs); TEST_ASSERT_EQUAL_FLOAT(11.2f, w.fcWindGustMs);
  TEST_ASSERT_EQUAL_STRING("", w.cityName);
}
void test_parse_night_icon() {
  char b[1200]; sample(b, sizeof b, 0, HOURLY7);
  WeatherData w; memset(&w, 0, sizeof w);
  TEST_ASSERT_TRUE(parse(b, w)); TEST_ASSERT_EQUAL_STRING("01n", w.iconCode);
}
void test_parse_error_body_rejected() {
  WeatherData w; memset(&w, 0, sizeof w); w.tempC = 5.0f;
  TEST_ASSERT_FALSE(parse("{\"error\":true,\"reason\":\"x\"}", w));
  TEST_ASSERT_FALSE(parse("[1]", w));
  TEST_ASSERT_FALSE(parse("{\"current\":{}}", w));
  TEST_ASSERT_EQUAL_FLOAT(5.0f, w.tempC); TEST_ASSERT_FALSE(w.detailValid[0]);
}
void test_parse_short_hourly() {
  const char* h4 = "\"hourly\":{\"temperature_2m\":[1,2,3,4],\"relative_humidity_2m\":[1,2,3,4],"
    "\"weather_code\":[0,0,0,0],\"wind_speed_10m\":[1,2,3,4],\"wind_gusts_10m\":[1,2,3,4]}";
  char b[1200]; sample(b, sizeof b, 1, h4);
  WeatherData w; memset(&w, 0, sizeof w);
  TEST_ASSERT_TRUE(parse(b, w));
  TEST_ASSERT_TRUE(w.detailValid[0]); TEST_ASSERT_FALSE(w.detailValid[1]);
  TEST_ASSERT_TRUE(w.detailValid[2]);
}
void test_parse_air_only() {
  JsonDocument d; deserializeJson(d, "{\"current\":{\"pm2_5\":31.8,\"pm10\":32.0}}");
  WeatherData w; memset(&w, 0, sizeof w);
  TEST_ASSERT_TRUE(openMeteoParse(d, w, true));
  TEST_ASSERT_EQUAL_FLOAT(31.8f, w.pm25); TEST_ASSERT_EQUAL_FLOAT(32.0f, w.pm10);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, w.tempC); TEST_ASSERT_FALSE(w.detailValid[0]);
  JsonDocument e; deserializeJson(e, "{\"error\":true}");
  TEST_ASSERT_FALSE(openMeteoParse(e, w, true));
}
void test_desc_fallback() {
  TEST_ASSERT_EQUAL((int)Tr::WxCondUnknown, (int)wxOmDescTr(0));
  TEST_ASSERT_EQUAL((int)Tr::WxCondThunder, (int)wxOmDescTr(wmoToOwmId(95)));
  TEST_ASSERT_EQUAL((int)Tr::WxCondClear, (int)wxOmDescTr(800));
}
int main() {
  UNITY_BEGIN();
  RUN_TEST(test_wmo_known_codes); RUN_TEST(test_wmo_unknown_is_clear); RUN_TEST(test_parse_sample);
  RUN_TEST(test_parse_night_icon); RUN_TEST(test_parse_error_body_rejected); RUN_TEST(test_parse_short_hourly);
  RUN_TEST(test_parse_air_only); RUN_TEST(test_desc_fallback);
  return UNITY_END();
}
