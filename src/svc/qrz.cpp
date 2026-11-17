#include "svc/qrz.h"
#include "app/boot_log.h"

#include <Arduino.h>
#include <WiFi.h>
#include <string.h>

#include "app/config_store.h"
#include "app/data_store.h"
#include "app/service_coords.h"
#include "geo.h"
#include "net/http_fetch.h"
#include "qrz_cache.h"
#include "url_encode.h"

constexpr uint32_t SESSION_TTL_MS = 30UL * 60UL * 1000UL;
constexpr uint32_t QRZ_HTTP_TIMEOUT_MS = 8000;

static QrzCache s_cache;
static QrzCache s_neg;  // not-found calls (empty info), same 15 min TTL
static QrzQueue s_queue;
static uint32_t s_loginFailAt;
static bool s_loginFailed;
static SemaphoreHandle_t s_mu;  // guards cache+queue: spots may be enqueued from other tasks
static char s_key[40];
static uint32_t s_keyAt;
static char s_status[40] = "disabled";

static void setStatus(const char* s) { strlcpy(s_status, s, sizeof s_status); }

// Net/other tasks must not touch cfg() (UI-only snapshot): read the credentials under the config mutex.
static bool enabled() {
  bool on = false;
  cfgRead([&](const Config& c) { on = c.qrz_user[0] && c.qrz_pass[0]; });
  return on;
}

static SemaphoreHandle_t mu() { return s_mu; }

void qrzInit() {
  s_mu = xSemaphoreCreateMutex();
  configASSERT(s_mu != nullptr);
}

static bool login(uint32_t now) {
  char u[100], p[100], url[256];
  cfgRead([&](const Config& c) {  // one consistent user/pass pair even while Settings writes
    urlEncode(c.qrz_user, u, sizeof u);
    urlEncode(c.qrz_pass, p, sizeof p);
  });
  snprintf(url, sizeof url, "https://xmldata.qrz.com/xml/current/?username=%s;password=%s", u, p);
  if (!httpGet(url, httpBody, kHttpBodySize, QRZ_HTTP_TIMEOUT_MS)) { setStatus("error: login http"); s_loginFailed = true; s_loginFailAt = now; return false; }
  if (!qrzParseSessionKey(httpBody, s_key, sizeof s_key)) {
    s_key[0] = 0;
    s_loginFailed = true;
    s_loginFailAt = now;
    setStatus("error: login failed");
    return false;
  }
  s_keyAt = now;
  s_loginFailed = false;
  Serial.println("[QRZ] session ok");
  static bool logged;
  if (!logged) { logged = true; bootLog("QRZ login ok"); }
  return true;
}

static void applyToSpot(Spot& s, const QrzInfo& q, bool haveUser, double ulat, double ulon) {
  if (q.grid[0]) strlcpy(s.loc, q.grid, sizeof s.loc);
  if (q.country[0]) strlcpy(s.country, q.country, sizeof s.country);
  double lat = q.lat, lon = q.lon;
  bool has = q.hasLatLon || (strlen(s.loc) >= 4 && locatorToLatLon(s.loc, lat, lon));
  if (!has) return;
  if (haveUser) s.distKm = (int16_t)distanceKm(ulat, ulon, lat, lon);
}

template <size_t N>
static void applyTo(Guarded<RingBuffer<Spot, N>>& g, const char* call, const QrzInfo& q, bool haveUser, double ulat,
                    double ulon) {
  bool match = false;
  g.read([&](const RingBuffer<Spot, N>& rb) {
    for (size_t i = 0; i < rb.size() && !match; ++i) match = strcasecmp(rb.at(i).call, call) == 0;
  });
  if (!match) return;  // avoid a needless version bump / UI refresh
  g.write([&](RingBuffer<Spot, N>& rb) {
    for (size_t i = 0; i < rb.size(); ++i)
      if (strcasecmp(rb.at(i).call, call) == 0) applyToSpot(rb.at(i), q, haveUser, ulat, ulon);
  });
}

static void applyToStores(const char* call, const QrzInfo& q) {
  double ulat = 0, ulon = 0;
  bool haveUser = serviceCoords(ulat, ulon);
  applyTo(g_store.dx, call, q, haveUser, ulat, ulon);
  applyTo(g_store.pota, call, q, haveUser, ulat, ulon);
}

static void qrzEnqueue(const char* call) {
  if (!enabled() || !call) return;
  xSemaphoreTake(mu(), portMAX_DELAY);
  s_queue.enqueue(call, millis());
  xSemaphoreGive(mu());
}

void qrzOnNewSpot(const Spot& s) {
  char n[QRZ_CALL_LEN];
  QrzInfo q;
  if (s.country[0] || !enabled() || !qrzNormalizeCall(s.call, n)) return;
  xSemaphoreTake(mu(), portMAX_DELAY);
  uint32_t now = millis();
  bool cached = s_cache.get(n, now, q) || s_neg.get(n, now, q);
  xSemaphoreGive(mu());
  if (!cached) qrzEnqueue(n);
}

const char* qrzStatusStr() { return s_status; }

void qrzTick(uint32_t now) {
  if (!enabled()) { setStatus("disabled"); return; }
  if (strcmp(s_status, "disabled") == 0) setStatus("ok");
  if (WiFi.status() != WL_CONNECTED) return;
  bool needLogin = !s_key[0] || now - s_keyAt >= SESSION_TTL_MS;
  if (needLogin && s_loginFailed && now - s_loginFailAt < 60000) return;  // login backoff
  char call[QRZ_CALL_LEN];
  xSemaphoreTake(mu(), portMAX_DELAY);
  bool due = s_queue.next(now, call);
  xSemaphoreGive(mu());
  if (!due || !httpPollGate(now)) return;  // gate after next(): a deferral only costs the 3 s rate slot

  QrzInfo info;
  bool ok = false, notFound = false;
  int fetches = 0;  // the first is already gated; later ones (login, then lookup) re-check
  auto mayFetch = [&] { return fetches++ == 0 || httpPollGate(now); };
  for (int attempt = 0; attempt < 2; ++attempt) {
    if (!s_key[0] || now - s_keyAt >= SESSION_TTL_MS) {
      if (!mayFetch() || !login(now)) break;
    }
    char url[128];
    snprintf(url, sizeof url, "https://xmldata.qrz.com/xml/current/?s=%s;callsign=%s", s_key, call);
    if (!mayFetch()) break;
    if (!httpGet(url, httpBody, kHttpBodySize, QRZ_HTTP_TIMEOUT_MS)) { setStatus("error: lookup http"); break; }
    if (strstr(httpBody, "Session Timeout") || strstr(httpBody, "Invalid session")) {
      s_key[0] = 0;  // re-login once
      if (attempt == 1) setStatus("error: session");
      continue;
    }
    ok = qrzParseXml(httpBody, info);
    notFound = !ok && strstr(httpBody, "<Error>Not found");  // other errors are retried, not cached
    setStatus(ok ? "ok" : "error: not found");
    break;
  }
  xSemaphoreTake(mu(), portMAX_DELAY);
  if (ok) { s_cache.put(call, info, now); s_queue.done(call); }
  else if (notFound) { QrzInfo none = {}; s_neg.put(call, none, now); s_queue.done(call); }
  else s_queue.fail(call, now);
  xSemaphoreGive(mu());
  if (ok) applyToStores(call, info);
}
