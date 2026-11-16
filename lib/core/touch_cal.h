#pragma once
#include <stdint.h>

struct TouchCal { float a, b, c, d, e, f; };  // x = a*rx + b*ry + c ; y = d*rx + e*ry + f

// Solves the affine map from 3 raw points to 3 screen points. False if the raw points are collinear.
bool touchCalSolve(const int16_t raw[3][2], const int16_t scr[3][2], TouchCal& out);
// Applies the map and clamps to 0..319 / 0..239.
void touchCalApply(const TouchCal&, int16_t rx, int16_t ry, int16_t& x, int16_t& y);
// Equivalent of the original map(200..3800) on both axes.
TouchCal touchCalDefault();
