#pragma once
#include <stdint.h>
#include <stddef.h>

// All shared POD data structs. Fixed char[] only: no String/std::string.
struct Spot {
  uint32_t utc; uint32_t khz;
  char call[12]; char comment[40]; char country[24]; char loc[7];
  int16_t distKm; uint8_t band, mode;
};

struct AprsStation {
  uint32_t utc; char call[12]; char sym[2]; char comment[48];
  float lat, lon; float freqMhz; int16_t distKm; int16_t bearing;
  bool hasLatLon; bool isWx;
  char source;  // 'I' APRS-IS, 'T' TNC2, 'K' KISS
};

// APRS message addressed to us. sym is the sender's last known symbol (two spaces if unknown); distKm is -1 if unknown.
struct AprsMessage {
  uint32_t utc; char from[12]; char text[68]; char msgNo[8]; char sym[2];
  bool hasLatLon; int16_t distKm;
};

// Decoded weather from an APRS comment (orig AprsWxDecoded L11870)
struct AprsWx {
  bool hasAny, hasWindDir, hasWindSpeed, hasTempC, hasHumidity, hasPressure;
  int windDirDeg; float windSpeedKmh; float tempC; int humidityPct; float pressureHpa;
};

struct PropagationData {
  char sfi[6], kindex[4], aindex[4], muf[8], updated[24];
  struct { char label[10]; char day[6]; char night[6]; } band[4];
  bool valid; uint32_t fetchedAt;
};

constexpr int WX_DETAIL_COLS = 5;
enum class WxDay : uint8_t { Unknown, Day, Night };  // Open-Meteo current.is_day; Unknown for OpenWeather
struct WeatherData {
  char cityName[32], description[40];
  int weatherId; float tempC, windMs, pm25, pm10; int humidity, pressure;
  uint32_t sunrise, sunset;  // OpenWeather sys.sunrise/sunset, UTC unix seconds; 0 = unknown
  WxDay day;
  float detailTempC[WX_DETAIL_COLS]; int detailHumidity[WX_DETAIL_COLS];
  float detailWindMs[WX_DETAIL_COLS]; int detailWeatherId[WX_DETAIL_COLS]; bool detailValid[WX_DETAIL_COLS];
  bool detailNight[WX_DETAIL_COLS];  // forecast slot is after sunset (OpenWeather city sunrise/sunset)
  float nightTempC[2]; bool nightTempValid[2];
  bool valid; uint32_t fetchedAt;
  bool fetchFailed, forecastFetchFailed;  // set by the service; screens map them to translated text
  // Forecast-derived alert inputs (next 6 h); stormEtaMin 0 = no storm. forecastFetchedAt (ms) is set by the service.
  int stormEtaMin; float fcWindMaxMs, fcWindGustMs; uint32_t forecastFetchedAt;
};

struct WeatherAlertInfo {
  bool stormPossible, stormNow, windPossible;
  uint8_t stormEtaHours;  // orig field actually carries repeat minutes
  float maxWindMs, maxWindGustMs;
};

// potaFail: the last POTA fetch failed (cleared by the next success); roundDone: the first boot HTTPS round is over.
struct NetStatus { bool sta; bool ap; char ip[16]; char ssid[33]; int8_t rssi; bool potaFail; bool roundDone; };

struct BeaconInfo { char call[12]; char position[24]; char path[32]; char sym[2]; };

struct QrzInfo { char grid[7]; char country[24]; float lat, lon; bool hasLatLon; };

struct AlertCfg {
  char watchList[96]; bool alertEnabled, rangeEnabled, wxEnabled, ledEnabled;
  float alertKm; uint16_t minSeconds;
};

struct WxAlertCfg { bool storm, wind; float windThresholdMs; };
