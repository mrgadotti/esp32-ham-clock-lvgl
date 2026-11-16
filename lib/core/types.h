#pragma once
#include <stdint.h>
#include <stddef.h>

// All shared POD data structs. Fixed char[] only: no String/std::string.
struct Spot {
  uint32_t utc; uint32_t khz;
  char call[12]; char spotter[12]; char comment[40]; char country[24]; char loc[7];
  float lat, lon; int16_t distKm; uint8_t band, mode; bool hasLatLon;
};

struct AprsStation {
  uint32_t utc; char call[12]; char sym[2]; char comment[48];
  float lat, lon; float freqMhz; int16_t distKm; int16_t bearing;
  bool hasLatLon; bool isWx;
  char source;  // 'I' APRS-IS, 'T' TNC2, 'K' KISS
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
struct WeatherData {
  char cityName[32], description[40], iconCode[8];
  char forecast3hDesc[40], forecastNextDayDesc[40], forecastError[48], updated[24], lastError[48];
  char detailIconCode[WX_DETAIL_COLS][8];
  int weatherId; float tempC, windMs, pm25, pm10; int humidity, pressure;
  float forecast3hTempC, forecast3hWindMs; bool forecast3hValid;
  float forecastNextDayTempC, forecastNextDayWindMs; bool forecastNextDayValid;
  float detailTempC[WX_DETAIL_COLS]; int detailHumidity[WX_DETAIL_COLS];
  float detailWindMs[WX_DETAIL_COLS]; int detailWeatherId[WX_DETAIL_COLS]; bool detailValid[WX_DETAIL_COLS];
  float nightTempC[2]; bool nightTempValid[2];
  bool valid; uint32_t fetchedAt;
};

struct WeatherAlertInfo {
  bool stormPossible, stormNow, windPossible;
  uint8_t stormEtaHours;  // orig field actually carries repeat minutes
  float maxWindMs, maxWindGustMs;
};

struct GpsState {
  bool active, fix; double lat, lon; float speedKmh;
  uint8_t sats, satsGps, satsGlo, satsGal, satsBds;
  uint32_t utc; uint32_t lastFixMs;
};

struct NetStatus { bool sta; bool ap; char ip[16]; char ssid[33]; int8_t rssi; };

struct BeaconInfo { char call[12]; char position[24]; char path[32]; };

struct QrzInfo { char grid[7]; char country[24]; float lat, lon; bool hasLatLon; };

struct AlertCfg {
  char watchList[96]; bool alertEnabled, rangeEnabled, wxEnabled, ledEnabled;
  float alertKm; uint16_t minSeconds;
};

struct WxAlertCfg { bool storm, wind; float windThresholdMs; };
