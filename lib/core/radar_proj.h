#pragma once
#include <stdint.h>
struct RadarView { float zoom; float cxKm, cyKm; };  // view centre in km (E, N) from QTH
RadarView radarReset();
// false if the pixel falls outside the 320x240 display
bool radarProject(const RadarView& v, float distKm, float bearingDeg, float rangeKm, int16_t radiusPx,
                  int16_t originX, int16_t originY, int16_t& px, int16_t& py);
// x2 zoom (max 8), re-centred on the tapped point
RadarView radarZoomAt(const RadarView& v, int16_t tapX, int16_t tapY, float rangeKm, int16_t radiusPx,
                      int16_t originX, int16_t originY);
