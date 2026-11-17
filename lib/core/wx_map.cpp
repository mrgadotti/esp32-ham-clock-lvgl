#include "wx_map.h"

// Same mapping as the original weatherIconPathForId.
WxIcon wxIconForId(int id, bool night) {
  if (id >= 200 && id <= 232) return WxIcon::I200;
  if (id >= 300 && id <= 321) return WxIcon::I300;
  if (id >= 500 && id <= 531) {
    if (id == 500 || id == 501) return WxIcon::I500;
    if (id == 502 || id == 503) return WxIcon::I502;
    if (id == 511 || id == 531) return WxIcon::I511;
    return WxIcon::I520;
  }
  if (id >= 600 && id <= 622) return (id == 600 || id == 601 || id == 602 || id >= 620) ? WxIcon::I600 : WxIcon::I611;
  if (id >= 700 && id <= 781) return WxIcon::I700;
  if (id == 800) return night ? WxIcon::I800n : WxIcon::I800;
  if (id == 801 || id == 802) return night ? WxIcon::I801n : WxIcon::I801;
  if (id == 803 || id == 804) return WxIcon::I803;
  return WxIcon::None;
}

int wxWindLevel(float ms) { return ms < 5 ? 0 : ms < 10 ? 1 : ms < 15 ? 2 : 3; }

int wxPmLevel(float v, float a, float b, float c, float d) { return v <= a ? 0 : v <= b ? 1 : v <= c ? 2 : v <= d ? 3 : 4; }
