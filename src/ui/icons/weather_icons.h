#pragma once
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif
extern const lv_image_dsc_t wxicon_200;
extern const lv_image_dsc_t wxicon_300;
extern const lv_image_dsc_t wxicon_500;
extern const lv_image_dsc_t wxicon_502;
extern const lv_image_dsc_t wxicon_511;
extern const lv_image_dsc_t wxicon_520;
extern const lv_image_dsc_t wxicon_600;
extern const lv_image_dsc_t wxicon_611;
extern const lv_image_dsc_t wxicon_700;
extern const lv_image_dsc_t wxicon_800;
extern const lv_image_dsc_t wxicon_800n;
extern const lv_image_dsc_t wxicon_801;
extern const lv_image_dsc_t wxicon_801n;
extern const lv_image_dsc_t wxicon_803;
extern const lv_image_dsc_t wxicon_wind;
#ifdef __cplusplus
}
#endif
// Maps an OpenWeather condition id to an icon; nullptr when unknown.
#ifdef __cplusplus
const lv_image_dsc_t* weatherIcon(int id, bool night);
#endif
