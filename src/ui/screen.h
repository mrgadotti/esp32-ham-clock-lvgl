#pragma once
#include <stdint.h>
#include "lvgl.h"

enum class ScreenId : uint8_t { HamClock=1, DxCluster=2, SunSpots=3, BandInfo=4, Weather=5, AprsList=6,
  Pota=7, HamAlert=8, AprsRadar=9, WeatherOm=10, ForecastOm=11, Forecast=12, Settings=14 };

class Screen {
 public:
  virtual ~Screen() = default;
  virtual ScreenId id() const = 0;
  virtual void create(lv_obj_t* parent) = 0;  // build widgets
  virtual void tick(uint32_t nowMs) = 0;      // every 100 ms; refresh only if a store version changed
  // True while a menu or overlay is active. ui_manager polls it every 100 ms, before tick(), so a screen must keep it
  // true until a tick() after the gesture ended (not clear it on the next press), or a quick tap can still switch screens.
  virtual bool blocksSwipe() const { return false; }
};

Screen* createScreen(ScreenId);  // factory in src/ui/screens/registry.cpp
