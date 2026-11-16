#include "aprs_beacon.h"

#include <Arduino.h>
#include <ctype.h>
#include <string.h>

#include "ax25_kiss.h"
#include "app/config_store.h"
#include "app/events.h"
#include "app/service_coords.h"
#include "svc/aprs_is.h"

// Task 17 provides the real UART2 KISS writer; the weak default means "no TNC".
__attribute__((weak)) bool uart2SendKiss(const uint8_t*, size_t) { return false; }

namespace {
constexpr uint32_t kFirstDelayMs = 60000;
constexpr const char* kProjectComment = "https://github.com/SP3KON/ESP32-HAM-CLOCK";
const char* const kPath[] = {"WIDE1-1", "WIDE2-1"};

uint32_t due;  // 0 = channel not ready
uint32_t count;
}  // namespace

void aprsBeaconTick(uint32_t now) {
  Config c;
  cfgGet(c);
  bool kiss = strcmp(c.uart2_cmod, "aprs_tnc_kiss") == 0;
  double lat = 0, lon = 0;
  bool ready = c.aprs_beacon && aprsTxCallValid(c.aprs_callsign) && serviceCoords(lat, lon) &&
               (kiss || aprsIsReadyToSend());
  if (!ready) {
    due = 0;
    count = 0;
    return;
  }
  if (!due) {
    due = now + kFirstDelayMs;
    return;
  }
  if ((int32_t)(now - due) < 0) return;
  due = now + (uint32_t)c.aprs_int_min * 60000u;

  char call[12];
  if (c.aprs_ssid > 0) snprintf(call, sizeof call, "%s-%d", c.aprs_callsign, (int)c.aprs_ssid);
  else snprintf(call, sizeof call, "%s", c.aprs_callsign);
  for (char* p = call; *p; p++) *p = (char)toupper((unsigned char)*p);

  // Leading space separates position from comment; every 5th beacon carries the project URL.
  char cm[64] = " ";
  const char* src = ((count + 1) % 5 == 0) ? kProjectComment : c.aprs_comment;
  size_t n = 1;
  for (; *src && n < sizeof cm - 1; src++)
    if ((unsigned char)*src >= 32 && (unsigned char)*src < 127) cm[n++] = *src;
  cm[n] = 0;

  char sym[3] = {'/', '-', 0};  // table, code
  if (c.aprs_symbol[0] && c.aprs_symbol[1]) { sym[0] = c.aprs_symbol[0]; sym[1] = c.aprs_symbol[1]; }
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
  Serial.printf("[APRS] beacon sent (%s)\n", pathStr);

  UiEvent e{};
  e.kind = EventKind::Beaconing;
  snprintf(e.beacon.call, sizeof e.beacon.call, "%s", call);
  snprintf(e.beacon.position, sizeof e.beacon.position, "%.4f %.4f", lat, lon);
  snprintf(e.beacon.path, sizeof e.beacon.path, "%s", pathStr);
  postUiEvent(e);
}
