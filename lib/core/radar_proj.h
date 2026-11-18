#pragma once
#include <stdint.h>
struct RadarView { float zoom; float cxKm, cyKm; };  // view centre in km (E, N) from QTH
RadarView radarReset();
// false if the pixel falls outside the 320x240 display
bool radarProject(const RadarView& v, float distKm, float bearingDeg, float rangeKm, int16_t radiusPx,
                  int16_t originX, int16_t originY, int16_t& px, int16_t& py);
// Next preset zoom (1,2,4,8,10,15,20,30,40,50,60,80) up (dir > 0) or down (dir < 0); the view centre is kept
RadarView radarZoomStep(const RadarView& v, int dir);
// x2 zoom (max 80), re-centred on the tapped point; unchanged at max zoom
RadarView radarZoomAt(const RadarView& v, int16_t tapX, int16_t tapY, float rangeKm, int16_t radiusPx,
                      int16_t originX, int16_t originY);
// Shifts the view centre by (dxKm, dyKm) and keeps |cx|, |cy| <= rangeKm so the outer ring stays reachable
RadarView radarPan(const RadarView& v, float dxKm, float dyKm, float rangeKm);
