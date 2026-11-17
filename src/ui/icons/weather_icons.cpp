#include "weather_icons.h"

#include "wx_map.h"

// Indexed by WxIcon (lib/core/wx_map.h).
const lv_image_dsc_t* weatherIcon(int id, bool night) {
  static const lv_image_dsc_t* const k[] = {nullptr,       &wxicon_200,  &wxicon_300, &wxicon_500,  &wxicon_502,
                                            &wxicon_511,   &wxicon_520,  &wxicon_600, &wxicon_611,  &wxicon_700,
                                            &wxicon_800,   &wxicon_800n, &wxicon_801, &wxicon_801n, &wxicon_803};
  return k[(int)wxIconForId(id, night)];
}
