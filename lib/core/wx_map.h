#pragma once
// Pure weather classification; src/ui maps the results to lv_image_dsc_t pointers and lv_color_t.

// OpenWeather condition id -> icon asset (wxicon_<n>); None when unknown.
enum class WxIcon : unsigned char { None, I200, I300, I500, I502, I511, I520, I600, I611, I700, I800, I800n, I801, I801n, I803 };
WxIcon wxIconForId(int id, bool night);

// Wind speed: 0 <5 m/s, 1 <10, 2 <15, 3 >= 15.
int wxWindLevel(float ms);
// Particulate: 0 v<=a, 1 <=b, 2 <=c, 3 <=d, 4 above d.
int wxPmLevel(float v, float a, float b, float c, float d);
