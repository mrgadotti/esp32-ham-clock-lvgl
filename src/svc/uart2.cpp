#include "uart2.h"

#include <Arduino.h>
#include <HardwareSerial.h>
#include <atomic>
#include <freertos/semphr.h>
#include <string.h>
#include <sys/time.h>

#include "app/config_store.h"
#include "app/data_store.h"
#include "aprs_ingest.h"
#include "ax25_kiss.h"
#include "board.h"
#include "bridge_proto.h"
#include "gps_time.h"
#include "nmea.h"
#include "net/time_service.h"

namespace {
enum Mode : uint8_t { M_NONE, M_GPS, M_TNC, M_KISS, M_BRIDGE };
struct PortCfg { Mode mode; int32_t baud, dbits, stop; char par[6]; };

SemaphoreHandle_t g_mu;             // guards g_fsm
BridgeFsm* g_fsm;                   // heap, only in bridge mode
std::atomic<Mode> g_mode{M_NONE};
std::atomic<bool> g_pairPending{false};
std::atomic<uint32_t> g_timeSetMs{0};  // millis() of the last settimeofday, 0 = never

Mode modeOf(const char* c) {
  if (!strcmp(c, "gps")) return M_GPS;
  if (!strcmp(c, "aprs_tnc")) return M_TNC;
  if (!strcmp(c, "aprs_tnc_kiss")) return M_KISS;
  if (!strcmp(c, "esp32_bridge")) return M_BRIDGE;
  return M_NONE;
}

uint32_t serialCfg(const PortCfg& p) {
  static const uint32_t t[] = {SERIAL_7N1, SERIAL_7N2, SERIAL_7E1, SERIAL_7E2, SERIAL_7O1, SERIAL_7O2,
                               SERIAL_8N1, SERIAL_8N2, SERIAL_8E1, SERIAL_8E2, SERIAL_8O1, SERIAL_8O2};
  int par = p.par[0] == 'e' ? 1 : p.par[0] == 'o' ? 2 : 0;
  return t[(p.dbits == 7 ? 0 : 6) + par * 2 + (p.stop == 2 ? 1 : 0)];
}

void readPortCfg(PortCfg& p) {
  static Config c;  // uart task only; too big for its stack
  cfgGet(c);
  p.mode = modeOf(c.uart2_cmod);
  p.baud = c.uart2_baud;
  p.dbits = c.uart2_dbits;
  p.stop = c.uart2_stop;
  strlcpy(p.par, c.uart2_par, sizeof p.par);
}

bool samePort(const PortCfg& a, const PortCfg& b) {
  return a.mode == b.mode && a.baud == b.baud && a.dbits == b.dbits && a.stop == b.stop && !strcmp(a.par, b.par);
}

void syncClock(const GpsState& g, uint32_t now) {
  if (!g.fix || !g.date || !g.utc) return;
  uint32_t set = g_timeSetMs;
  if (set && now - set < 60000UL) return;
  if (ntpSyncedWithin(2UL * 3600 * 1000)) return;
  int64_t t;
  if (!gpsToUnix(g.date, g.utc, t)) return;
  struct timeval tv = {(time_t)t, 0};
  settimeofday(&tv, nullptr);
  g_timeSetMs = now | 1;
}

char g_saveAddr[13], g_saveName[41];  // cfgUpdate takes a plain function pointer
void saveBridgePair() {
  cfgUpdate([](Config& c) {
    strlcpy(c.br_pair_addr, g_saveAddr, sizeof c.br_pair_addr);
    strlcpy(c.br_pair_name, g_saveName, sizeof c.br_pair_name);
  });
}
}  // namespace

bool gpsTimeValid() {
  uint32_t t = g_timeSetMs;
  return t && millis() - t < 180000UL;
}

bool uart2SendKiss(const uint8_t* frame, size_t len) {
  if (!g_mu || xSemaphoreTake(g_mu, pdMS_TO_TICKS(200)) != pdTRUE) return false;
  bool ok = g_mode == M_KISS && Serial2.write(frame, len) == len;  // mutex: port cannot close mid-write
  xSemaphoreGive(g_mu);
  return ok;
}

bool uart2BridgeEnabled() { return g_mode == M_BRIDGE; }

void uart2BridgeStatus(JsonDocument& d) {
  // Copy under the lock, build the JSON (heap allocations) outside it: the uart task waits on g_mu forever.
  struct Snap {
    bool have, scan, pairPending;
    int pairResult, count;
    char lastError[40], lastAck[24], lastDisc[16], pairedName[41], pairedAddr[13];
    BridgeDevice devices[BRIDGE_MAX_DEVICES];
  };
  static Snap sn;  // web task only; too big for its stack
  d["enabled"] = uart2BridgeEnabled();
  if (!g_mu || xSemaphoreTake(g_mu, pdMS_TO_TICKS(200)) != pdTRUE) return;
  sn.have = g_fsm != nullptr;
  if (sn.have) {
    const BridgeFsm& f = *g_fsm;
    sn.scan = f.scanInProgress;
    sn.pairPending = g_pairPending;
    sn.pairResult = f.pairResult;
    sn.count = f.deviceCount;
    memcpy(sn.lastError, f.lastError, sizeof sn.lastError);
    memcpy(sn.lastAck, f.lastAck, sizeof sn.lastAck);
    memcpy(sn.lastDisc, f.lastDisc, sizeof sn.lastDisc);
    memcpy(sn.pairedName, f.pairedName, sizeof sn.pairedName);
    memcpy(sn.pairedAddr, f.pairedAddr, sizeof sn.pairedAddr);
    memcpy(sn.devices, f.devices, sizeof(BridgeDevice) * f.deviceCount);
  }
  xSemaphoreGive(g_mu);
  if (!sn.have) return;
  d["scan_in_progress"] = sn.scan;
  d["last_error"] = sn.lastError;
  d["last_ack"] = sn.lastAck;
  d["disc_status"] = sn.lastDisc;
  d["pair_pending"] = sn.pairPending;
  d["pair_result"] = sn.pairResult > 0 ? "ok" : sn.pairResult < 0 ? "err" : "none";
  d["paired_name"] = sn.pairedName;
  d["paired_addr"] = sn.pairedAddr;
  JsonArray a = d["devices"].to<JsonArray>();
  for (int i = 0; i < sn.count; i++) {
    JsonObject o = a.add<JsonObject>();
    o["name"] = sn.devices[i].name;
    o["addr_raw"] = sn.devices[i].addr;
    char s[18];
    const char* r = sn.devices[i].addr;
    snprintf(s, sizeof s, "%.2s:%.2s:%.2s:%.2s:%.2s:%.2s", r, r + 2, r + 4, r + 6, r + 8, r + 10);
    o["addr"] = s;
  }
}

bool uart2BridgeScan() {
  if (!g_mu || !uart2BridgeEnabled() || xSemaphoreTake(g_mu, pdMS_TO_TICKS(200)) != pdTRUE) return false;
  bool ok = g_fsm != nullptr;
  if (ok) g_fsm->startScan();
  xSemaphoreGive(g_mu);
  return ok;
}

bool uart2BridgePair(const char* name, const char* addr, const char* password) {
  if (!g_mu || !uart2BridgeEnabled() || xSemaphoreTake(g_mu, pdMS_TO_TICKS(200)) != pdTRUE) return false;
  bool ok = g_fsm && g_fsm->pair(name, addr, password);
  if (ok) g_pairPending = true;
  xSemaphoreGive(g_mu);
  return ok;
}

void uart2Task(void*) {
  g_mu = xSemaphoreCreateMutex();
  configASSERT(g_mu != nullptr);
  PortCfg cur{};
  GpsState gps{}, pub{};
  KissRx* kiss = nullptr;
  static char line[400];
  size_t ln = 0;
  uint32_t lastCfg = 0, lastSentence = 0;
  bool open = false, first = true;

  for (;;) {
    uint32_t now = millis();
    if (first || now - lastCfg >= 2000) {
      lastCfg = now;
      PortCfg n;
      readPortCfg(n);
      if (first || !samePort(n, cur)) {
        first = false;
        xSemaphoreTake(g_mu, portMAX_DELAY);
        g_mode = M_NONE;
        if (open) Serial2.end();
        xSemaphoreGive(g_mu);
        open = false;
        cur = n;
        ln = 0;
        gps = GpsState{};
        xSemaphoreTake(g_mu, portMAX_DELAY);
        delete g_fsm;
        g_fsm = cur.mode == M_BRIDGE ? new BridgeFsm() : nullptr;
        if (g_fsm) {
          static Config sc;
          cfgGet(sc);
          strlcpy(g_fsm->pairedAddr, sc.br_pair_addr, sizeof g_fsm->pairedAddr);
          strlcpy(g_fsm->pairedName, sc.br_pair_name, sizeof g_fsm->pairedName);
          g_fsm->autoConnectPending = true;
        }
        g_pairPending = false;
        xSemaphoreGive(g_mu);
        if (cur.mode == M_KISS && !kiss) kiss = new KissRx();
        if (cur.mode != M_KISS && kiss) { delete kiss; kiss = nullptr; }
        if (cur.mode != M_NONE) {
          Serial2.setRxBufferSize(1024);  // must precede begin()
          Serial2.begin(cur.baud, serialCfg(cur), PIN_UART2_RX, PIN_UART2_TX);
          open = true;
        }
        g_mode = cur.mode;  // published only once the port is open
      }
    }

    if (!open) {
      vTaskDelay(pdMS_TO_TICKS(100));
    } else {
      for (int n = 0; n < 256 && Serial2.available(); n++) {
        uint8_t b = Serial2.read();
        if (cur.mode == M_KISS) {
          if (kissDeframe(b, *kiss)) {
            char tnc2[300];
            if (ax25ToTnc2(kiss->buf, kiss->len, tnc2, sizeof tnc2)) aprsIngestLine(tnc2, 'K');
          }
          continue;
        }
        if (b != '\r' && b != '\n') {
          if (ln < sizeof line - 1) line[ln++] = b;
          else ln = 0;  // overlong: drop
          continue;
        }
        if (!ln) continue;
        line[ln] = 0;
        ln = 0;
        if (cur.mode == M_TNC) {
          aprsIngestLine(line, 'T');
        } else if (line[0] == '$') {
          if (nmeaParse(line, gps, now)) lastSentence = now;
        } else if (cur.mode == M_BRIDGE) {
          xSemaphoreTake(g_mu, portMAX_DELAY);
          if (g_fsm) g_fsm->onLine(line);
          xSemaphoreGive(g_mu);
        }
      }

      if (cur.mode == M_BRIDGE) {
        char cmd[BRIDGE_CMD_MAX];
        bool have = false;
        bool save = false;
        xSemaphoreTake(g_mu, portMAX_DELAY);
        if (g_fsm) {
          g_fsm->tick(now);
          have = g_fsm->popCommand(cmd, sizeof cmd);
          if (g_pairPending && g_fsm->pairResult != 0) {
            g_pairPending = false;
            if (g_fsm->pairResult > 0) {
              strlcpy(g_saveAddr, g_fsm->pairedAddr, sizeof g_saveAddr);
              strlcpy(g_saveName, g_fsm->pairedName, sizeof g_saveName);
              save = true;
            }
          }
        }
        xSemaphoreGive(g_mu);
        if (save) saveBridgePair();
        if (have) Serial2.println(cmd);
      }

      if (cur.mode == M_GPS || cur.mode == M_BRIDGE) {
        if (now - lastSentence > 5000) gps.active = false;
        if (gps.fix && now - gps.lastFixMs > 15000) gps.fix = false;
        syncClock(gps, now);
      }
      vTaskDelay(pdMS_TO_TICKS(5));
    }

    if (memcmp(&pub, &gps, sizeof gps)) {
      pub = gps;
      g_store.gps.write([&](GpsState& g) { g = gps; });
    }
  }
}
