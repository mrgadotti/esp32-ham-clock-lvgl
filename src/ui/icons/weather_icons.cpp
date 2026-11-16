#include "weather_icons.h"

// Same mapping as the original weatherIconPathForId.
const lv_image_dsc_t* weatherIcon(int id, bool night) {
  if (id >= 200 && id <= 232) return &wxicon_200;
  if (id >= 300 && id <= 321) return &wxicon_300;
  if (id >= 500 && id <= 531) {
    if (id == 500 || id == 501) return &wxicon_500;
    if (id == 502 || id == 503) return &wxicon_502;
    if (id == 511 || id == 531) return &wxicon_511;
    return &wxicon_520;
  }
  if (id >= 600 && id <= 622) return (id == 600 || id == 601 || id == 602 || id >= 620) ? &wxicon_600 : &wxicon_611;
  if (id >= 700 && id <= 781) return &wxicon_700;
  if (id == 800) return night ? &wxicon_800n : &wxicon_800;
  if (id == 801 || id == 802) return night ? &wxicon_801n : &wxicon_801;
  if (id == 803 || id == 804) return &wxicon_803;
  return nullptr;
}
