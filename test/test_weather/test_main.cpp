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

void test_night_min_small_hours() {
  // Nov 17 (tz 0): 00:00=5, 03:00=3.5, 06:00=4, 21:00=6 -> 03:00 wins
  JsonDocument d;
  deserializeJson(d,
    "{\"cod\":200,\"list\":["
    "{\"dt\":1794841200,\"main\":{\"temp\":10,\"humidity\":1},\"weather\":[{\"id\":800,\"icon\":\"01d\"}],\"wind\":{\"speed\":1}},"
    "{\"dt\":1794873600,\"main\":{\"temp\":5.0},\"weather\":[{\"id\":800,\"icon\":\"01n\"}]},"
    "{\"dt\":1794884400,\"main\":{\"temp\":3.5},\"weather\":[{\"id\":800,\"icon\":\"01n\"}]},"
    "{\"dt\":1794895200,\"main\":{\"temp\":4.0},\"weather\":[{\"id\":800,\"icon\":\"01n\"}]},"
    "{\"dt\":1794949200,\"main\":{\"temp\":6.0},\"weather\":[{\"id\":800,\"icon\":\"01n\"}]},"
    "42,{\"main\":{\"temp\":99}}]}");
  WeatherData w; memset(&w, 0, sizeof w);
  TEST_ASSERT_TRUE(weatherParseForecast(d, w, NOW, 0));
  TEST_ASSERT_TRUE(w.nightTempValid[0]); TEST_ASSERT_EQUAL_FLOAT(3.5f, w.nightTempC[0]);
  deserializeJson(d, "{\"list\":[1,\"x\",{\"main\":{}}]}");
  WeatherData k; memset(&k, 0, sizeof k);
  TEST_ASSERT_FALSE(weatherParseForecast(d, k, NOW, 0)); TEST_ASSERT_FALSE(k.detailValid[0]);
}

void test_text_sanitizing() {
  JsonDocument d;
  WeatherData w; memset(&w, 0, sizeof w);
  // C2 85 (C1 control) dropped, lone continuation 0x80 dropped
  deserializeJson(d, "{\"weather\":[{\"id\":800,\"description\":\"a\xC2\x85" "b\x80" "c\",\"icon\":\"01d\"}],\"main\":{\"temp\":1}}");
  TEST_ASSERT_TRUE(weatherParseCurrent(d, w));
  TEST_ASSERT_EQUAL_STRING("abc", w.description);
  // 40 x U+00E9 into char[32]: whole sequences only -> 15 chars = 30 bytes
  char json[256] = "{\"name\":\"";
  for (int i = 0; i < 40; i++) strcat(json, "\xC3\xA9");
  strcat(json, "\",\"weather\":[{\"id\":800,\"icon\":\"01d\"}],\"main\":{\"temp\":1}}");
  deserializeJson(d, json);
  TEST_ASSERT_TRUE(weatherParseCurrent(d, w));
  TEST_ASSERT_EQUAL_INT(30, (int)strlen(w.cityName));
  TEST_ASSERT_EQUAL_UINT8(0xA9, (uint8_t)w.cityName[29]);
}

void test_text_drops_glyphs_missing_from_fonts() {
  JsonDocument d;
  WeatherData w; memset(&w, 0, sizeof w);
  // NBSP, inverted question mark, division sign dropped; degree sign, a-grave and the range ends C0/FF kept
  deserializeJson(d, "{\"name\":\"a\xC2\xA0" "b\xC2\xBF" "c\xC3\xB7" "d\xC2\xB0" "e\xC2\xA9" "f\xC3\x80" "g\xC3\xBF\",\"weather\":[{\"id\":800,\"icon\":\"01d\"}],\"main\":{\"temp\":1}}");
  TEST_ASSERT_TRUE(weatherParseCurrent(d, w));
  TEST_ASSERT_EQUAL_STRING("abcd\xC2\xB0" "ef\xC3\x80" "g\xC3\xBF", w.cityName);
}

void test_air_requires_pm() {
  JsonDocument d; WeatherData w; memset(&w, 0, sizeof w); w.pm25 = 7;
  deserializeJson(d, "{\"list\":[{\"components\":{\"co\":1}}]}");
  TEST_ASSERT_FALSE(weatherParseAir(d, w)); TEST_ASSERT_EQUAL_FLOAT(7, w.pm25);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_night_min_small_hours); RUN_TEST(test_text_sanitizing); RUN_TEST(test_text_drops_glyphs_missing_from_fonts); RUN_TEST(test_air_requires_pm);
  RUN_TEST(test_current_fixture); RUN_TEST(test_forecast_fixture);
  RUN_TEST(test_air_fixture); RUN_TEST(test_weather_rejects_garbage);
  return UNITY_END();
}
