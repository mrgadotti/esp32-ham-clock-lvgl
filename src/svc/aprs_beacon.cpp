#include "aprs_beacon.h"

#include <Arduino.h>
#include <atomic>
#include <ctype.h>
#include <string.h>

#include "ax25_kiss.h"
#include "app/config_store.h"
#include "app/events.h"
#include "app/service_coords.h"
#include "svc/aprs_is.h"
#include "svc/uart2.h"

namespace {
constexpr uint32_t kFirstDelayMs = 60000;
constexpr const char kProjectComment[] = "github.com/mrgadotti/esp32-ham-clock-lvgl";  // no scheme: the APRS comment limit is 43 chars
static_assert(sizeof kProjectComment - 1 <= 43, "APRS comment limit");
const char* const kPath[] = {"WIDE1-1", "WIDE2-1"};

struct Cfg {
  char cmod[20], call[12], comment[44], symbol[3];
  int32_t ssid, intMin;
  bool beacon;
} c;  // net task only; refreshed when cfgVersion() moves
uint32_t cfgVer;
bool haveCfg;

bool armed, everSent;  // armed: due is scheduled; cleared whenever the channel is not ready
std::atomic<bool> beaconReq;  // set by the UI task, consumed by the tick
uint32_t due, lastTx, count;  // lastTx/count survive reconnects so the cadence is kept
}  // namespace

void aprsBeaconRequest() { beaconReq = true; }

void aprsBeaconTick(uint32_t now) {
  uint32_t v = cfgVersion();
  if (!haveCfg || v != cfgVer) {
    haveCfg = true;
    cfgVer = v;
    cfgRead([](const Config& s) {
      strlcpy(c.cmod, s.uart2_cmod, sizeof c.cmod);
      strlcpy(c.call, s.aprs_callsign, sizeof c.call);
      strlcpy(c.comment, s.aprs_comment, sizeof c.comment);
      strlcpy(c.symbol, s.aprs_symbol, sizeof c.symbol);
      c.ssid = s.aprs_ssid; c.intMin = s.aprs_int_min; c.beacon = s.aprs_beacon;
    });
  }
  bool kiss = strcmp(c.cmod, "aprs_tnc_kiss") == 0;
  double lat = 0, lon = 0;
  bool ready = c.beacon && aprsTxCallValid(c.call) && serviceCoords(lat, lon) &&
               (kiss || aprsIsReadyToSend());
  bool force = beaconReq.exchange(false);  // a request while not ready is dropped
  if (!ready) {
    armed = false;
    return;
  }
  if (force) armed = true;
  else if (!armed) {  // wait 60 s after ready, and the rest of the interval since the last TX
    armed = true;
    due = now + kFirstDelayMs;
    uint32_t next = lastTx + (uint32_t)c.intMin * 60000u;
    if (everSent && (int32_t)(next - due) > 0) due = next;
    return;
  }
  if (!force && (int32_t)(now - due) < 0) return;
  due = now + (uint32_t)c.intMin * 60000u;

  char call[12];
  int cl = c.ssid > 0 ? snprintf(call, sizeof call, "%s-%d", c.call, (int)c.ssid)
                           : snprintf(call, sizeof call, "%s", c.call);
  if (cl < 0 || cl >= (int)sizeof call) return;
  for (char* p = call; *p; p++) *p = (char)toupper((unsigned char)*p);

  // Leading space separates position from comment; every 5th beacon carries the project URL.
  char cm[64] = " ";
  const char* src = ((count + 1) % 5 == 0) ? kProjectComment : c.comment;
  size_t n = 1;
  for (; *src && n < sizeof cm - 1; src++)
    if ((unsigned char)*src >= 32 && (unsigned char)*src < 127) cm[n++] = *src;
  cm[n] = 0;
  if (n == 1) cm[0] = 0;  // empty comment: no trailing space

  char sym[3] = {'/', '-', 0};  // table, code
  if (c.symbol[0] && c.symbol[1]) { sym[0] = c.symbol[0]; sym[1] = c.symbol[1]; }
  char info[96];
  if (!formatAprsPosition(lat, lon, sym[0], sym[1], cm, info, sizeof info)) return;

  bool ok;
  const char* pathStr;
  if (kiss) {
    uint8_t frame[160];
    size_t len = kissEncodeUiFrame(call, "APRS", kPath, 2, info, frame, sizeof frame);
    ok = len && uart2SendKiss(frame, len);
    pathStr = "APRS,WIDE1-1,WIDE2-1";
  } else {
    char line[160];
    snprintf(line, sizeof line, "%s>APRS,TCPIP*:%s", call, info);
    ok = aprsIsSendLine(line);
    pathStr = "APRS,TCPIP*";
  }
  if (!ok) {
    Serial.println("[APRS] beacon TX failed");
    return;
  }
  count++;
  lastTx = now;
  everSent = true;
  Serial.printf("[APRS] beacon sent (%s)\n", pathStr);

  UiEvent e{};
  e.kind = EventKind::Beaconing;
  snprintf(e.beacon.call, sizeof e.beacon.call, "%s", call);
  snprintf(e.beacon.position, sizeof e.beacon.position, "%.4f %.4f", lat, lon);
  snprintf(e.beacon.path, sizeof e.beacon.path, "%s", pathStr);
  postUiEvent(e);
}
