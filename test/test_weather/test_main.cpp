#include <unity.h>
#include <string.h>
#include "weather_json.h"
void setUp() {} void tearDown() {}

static const long NOW = 1794830400L;  // 2026-11-16 12:00 UTC

void test_current_fixture() {
  JsonDocument d;
  deserializeJson(d,
    "{\"coord\":{\"lon\":-46.63,\"lat\":-23.55},\"weather\":[{\"id\":501,\"main\":\"Rain\","
    "\"description\":\"chuva moderada \xC3\xA0s \xE2\x82\xAC\",\"icon\":\"10n\"}],\"main\":{\"temp\":18.4,"
    "\"pressure\":1012,\"humidity\":82},\"wind\":{\"speed\":3.6,\"deg\":200},\"name\":\"S\xC3\xA3o Paulo\",\"cod\":200}");
  WeatherData w; memset(&w, 0, sizeof w);
  TEST_ASSERT_TRUE(weatherParseCurrent(d, w));
  TEST_ASSERT_TRUE(w.valid);
  TEST_ASSERT_EQUAL_FLOAT(18.4f, w.tempC);
  TEST_ASSERT_EQUAL_INT(82, w.humidity); TEST_ASSERT_EQUAL_INT(1012, w.pressure);
  TEST_ASSERT_EQUAL_FLOAT(3.6f, w.windMs); TEST_ASSERT_EQUAL_INT(501, w.weatherId);
  TEST_ASSERT_EQUAL_STRING("10n", w.iconCode);
  TEST_ASSERT_EQUAL_STRING("S\xC3\xA3o Paulo", w.cityName);
  // Latin-1 kept as UTF-8, euro sign (outside Latin-1) dropped
  TEST_ASSERT_EQUAL_STRING("chuva moderada \xC3\xA0s ", w.description);
  TEST_ASSERT_TRUE(isNightFromIcon(w.iconCode));
  TEST_ASSERT_FALSE(isNightFromIcon("01d")); TEST_ASSERT_FALSE(isNightFromIcon(""));
}

void test_forecast_fixture() {
  JsonDocument d;
  deserializeJson(d,
    "{\"cod\":\"200\",\"cnt\":6,\"list\":["
    "{\"dt\":1794841200,\"main\":{\"temp\":10.0,\"humidity\":80},\"weather\":[{\"id\":211,\"icon\":\"11d\"}],\"wind\":{\"speed\":3.0,\"gust\":6.0}},"
    "{\"dt\":1794852000,\"main\":{\"temp\":8.0,\"humidity\":75},\"weather\":[{\"id\":801,\"icon\":\"02n\"}],\"wind\":{\"speed\":4.0,\"gust\":12.0}},"
    "{\"dt\":1794916800,\"main\":{\"temp\":9.0,\"humidity\":60},\"weather\":[{\"id\":803,\"icon\":\"04d\"}],\"wind\":{\"speed\":2.0}},"
    "{\"dt\":1794949200,\"main\":{\"temp\":6.0,\"humidity\":70},\"weather\":[{\"id\":800,\"icon\":\"01n\"}],\"wind\":{\"speed\":1.0}},"
    "{\"dt\":1794970800,\"main\":{\"temp\":2.0,\"humidity\":90},\"weather\":[{\"id\":800,\"icon\":\"01n\"}],\"wind\":{\"speed\":1.0}},"
    "{\"dt\":1795003200,\"main\":{\"temp\":7.0,\"humidity\":55},\"weather\":[{\"id\":600,\"icon\":\"13d\"}],\"wind\":{\"speed\":5.5}}"
    "],\"city\":{\"name\":\"Lisboa\"}}");
  WeatherData w; memset(&w, 0, sizeof w);
  TEST_ASSERT_TRUE(weatherParseForecast(d, w, NOW, 0));
  TEST_ASSERT_TRUE(w.detailValid[0]); TEST_ASSERT_EQUAL_FLOAT(10.0f, w.detailTempC[0]);
  TEST_ASSERT_EQUAL_INT(211, w.detailWeatherId[0]); TEST_ASSERT_EQUAL_STRING("11d", w.detailIconCode[0]);
  TEST_ASSERT_EQUAL_FLOAT(8.0f, w.detailTempC[1]);                       // +6h
  TEST_ASSERT_EQUAL_FLOAT(9.0f, w.detailTempC[2]);                       // tomorrow noon
  TEST_ASSERT_EQUAL_INT(60, w.detailHumidity[2]);
  TEST_ASSERT_EQUAL_FLOAT(7.0f, w.detailTempC[3]);                       // day after
  TEST_ASSERT_TRUE(w.nightTempValid[0]); TEST_ASSERT_EQUAL_FLOAT(6.0f, w.nightTempC[0]);
  TEST_ASSERT_TRUE(w.nightTempValid[1]); TEST_ASSERT_EQUAL_FLOAT(2.0f, w.nightTempC[1]);
  TEST_ASSERT_TRUE(w.forecast3hValid); TEST_ASSERT_TRUE(w.forecastNextDayValid);
  TEST_ASSERT_EQUAL_STRING("Lisboa", w.cityName);
  TEST_ASSERT_EQUAL_INT(180, w.stormEtaMin);                             // storm in +3h
  TEST_ASSERT_EQUAL_FLOAT(4.0f, w.fcWindMaxMs); TEST_ASSERT_EQUAL_FLOAT(12.0f, w.fcWindGustMs);
}

void test_air_fixture() {
  JsonDocument d;
  deserializeJson(d, "{\"coord\":{\"lon\":1,\"lat\":2},\"list\":[{\"main\":{\"aqi\":2},\"components\":{\"co\":200.5,\"pm2_5\":8.25,\"pm10\":14.5}}]}");
  WeatherData w; memset(&w, 0, sizeof w);
  TEST_ASSERT_TRUE(weatherParseAir(d, w));
  TEST_ASSERT_EQUAL_FLOAT(8.25f, w.pm25); TEST_ASSERT_EQUAL_FLOAT(14.5f, w.pm10);
}

void test_weather_rejects_garbage() {
  WeatherData w; memset(&w, 0, sizeof w);
  strcpy(w.cityName, "Keep"); w.valid = true; w.tempC = 5; w.pm25 = 3;
  JsonDocument d;
  deserializeJson(d, "{}");
  TEST_ASSERT_FALSE(weatherParseCurrent(d, w)); TEST_ASSERT_FALSE(weatherParseForecast(d, w, NOW, 0));
  TEST_ASSERT_FALSE(weatherParseAir(d, w));
  deserializeJson(d, "{\"cod\":401,\"message\":\"Invalid API key\"}");
  TEST_ASSERT_FALSE(weatherParseCurrent(d, w)); TEST_ASSERT_FALSE(weatherParseForecast(d, w, NOW, 0));
  deserializeJson(d, "{\"cod\":\"200\",\"list\":[]}");
  TEST_ASSERT_FALSE(weatherParseForecast(d, w, NOW, 0));
  TEST_ASSERT_EQUAL_STRING("Keep", w.cityName); TEST_ASSERT_EQUAL_FLOAT(5, w.tempC);
  TEST_ASSERT_EQUAL_FLOAT(3, w.pm25); TEST_ASSERT_FALSE(w.detailValid[0]);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_current_fixture); RUN_TEST(test_forecast_fixture);
  RUN_TEST(test_air_fixture); RUN_TEST(test_weather_rejects_garbage);
  return UNITY_END();
}
