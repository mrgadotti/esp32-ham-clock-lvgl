#include "touch.h"

#include <SPI.h>
#include <XPT2046_Touchscreen.h>

#include "board.h"

static SPIClass touchSpi(VSPI);
static XPT2046_Touchscreen ts(PIN_TOUCH_CS, PIN_TOUCH_IRQ);
static TouchCal cal;
static bool mirror;
static void (*gestureSink)(Gesture);
static lv_indev_t* indev;

void touchInit() {
  touchSpi.begin(PIN_TOUCH_CLK, PIN_TOUCH_MISO, PIN_TOUCH_MOSI, PIN_TOUCH_CS);
  ts.begin(touchSpi);
  ts.setRotation(1);
  cal = touchCalDefault();
}

void touchSetCal(const TouchCal& c) { cal = c; }
void touchSetMirror(bool m) { mirror = m; }
void touchSetGestureSink(void (*sink)(Gesture)) { gestureSink = sink; }

bool touchReadRaw(int16_t& rx, int16_t& ry) {
  if (!ts.tirqTouched()) return false;
  int32_t sx = 0, sy = 0;
  for (int i = 0; i < 4; i++) {
    TS_Point p = ts.getPoint();
    if (p.z < 400) return false;  // minimum pressure
    sx += p.x;
    sy += p.y;
  }
  rx = sx / 4;
  ry = sy / 4;
  return true;
}

bool touchHeldAtBoot(uint32_t ms) {
  uint32_t t0 = millis();
  int16_t x, y;
  while (millis() - t0 < ms) {
    if (!touchReadRaw(x, y)) return false;
    delay(20);
  }
  return true;
}

static void readCb(lv_indev_t*, lv_indev_data_t* data) {
  static bool down;
  static int16_t ox, oy, lx, ly;
  int16_t rx, ry, x, y;
  if (touchReadRaw(rx, ry)) {
    touchCalApply(cal, rx, ry, x, y);
    if (mirror) { x = 319 - x; y = 239 - y; }
    if (!down) { down = true; ox = x; oy = y; }
    lx = x;
    ly = y;
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
    if (down) {
      down = false;
      Gesture g = classifyGesture(lx - ox, ly - oy);
      if (g != Gesture::None) {
        if (gestureSink) gestureSink(g);
        lv_indev_wait_release(indev);  // swallow the click the release would produce
      }
    }
  }
  data->point.x = lx;
  data->point.y = ly;
}

lv_indev_t* touchCreateIndev() {
  indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, readCb);
  return indev;
}
