#include "web_config.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <WiFi.h>

#include "app/config_store.h"
#include "app/data_store.h"
#include "net/http_fetch.h"
#include "svc/qrz.h"
#include "svc/uart2.h"
#include "app/events.h"
#include "board.h"
#include "config_html.h"

#ifndef FW_VERSION
#define FW_VERSION "2.0.0-dev"
#endif

namespace {
const size_t kMaxBody = 6 * 1024;

WebServer server(80);
bool started = false;
uint32_t restartAt = 0;  // millis() deadline, 0 = none

// send_P with a length writes straight from our buffer; send(code, type, const char*) would copy it into a String.
void sendJson(int code, const char* s) { server.send_P(code, "application/json", s, strlen(s)); }

// CSRF guard for every POST. A cross-site page can only send "simple" requests (form-urlencoded, text/plain) without a
// preflight, which this server never answers, so requiring application/json blocks it. The Origin check (sent by
// browsers on cross-origin and same-origin POSTs) is a second layer; curl and other non-browser clients omit it.
bool postAllowed() {
  if (!server.header("Content-Type").startsWith("application/json")) {
    sendJson(415, "{\"error\":\"content_type_must_be_json\"}");
    return false;
  }
  String origin = server.header("Origin");
  if (origin.length() && origin != "http://" + server.hostHeader()) {
    sendJson(403, "{\"error\":\"bad_origin\"}");
    return false;
  }
  return true;
}

// Serializes into the shared net-task body buffer (handlers run one at a time in the net task, between fetches) and
// sends it: no String, no per-request malloc, and the same char* serializer configToJson already links.
void sendDoc(int code, const JsonDocument& d) {
  size_t n = serializeJson(d, httpBody, kHttpBodySize);
  if (n == 0 || n >= kHttpBodySize - 1) return sendJson(500, "{\"error\":\"json_too_large\"}");
  sendJson(code, httpBody);
}

void handleRoot() {
  server.sendHeader("Content-Encoding", "gzip");
  server.send_P(200, "text/html", (PGM_P)CONFIG_HTML_GZ, CONFIG_HTML_GZ_LEN);
}

void handleConfig() {
  Config c;
  cfgGet(c);
  if (configToJson(c, httpBody, kHttpBodySize)) sendJson(200, httpBody);
  else sendJson(500, "{\"error\":\"config_too_large\"}");
}

void handleSave() {
  if (!postAllowed()) return;
  const String& body = server.arg("plain");
  if (body.length() > kMaxBody) return sendJson(413, "{\"error\":\"body_too_large\"}");
  ApplyResult r;
  cfgApplyJson(body.c_str(), r);
  if (!r.ok) {
    JsonDocument d;
    d["error"] = r.error;
    return sendDoc(400, d);
  }
  if (r.needsRestart) restartAt = (millis() + 1500) | 1;  // |1 keeps it nonzero
  sendJson(200, r.needsRestart ? "{\"status\":\"ok\",\"restart\":true}" : "{\"status\":\"ok\",\"restart\":false}");
}

void handleResetTouch() {
  if (!postAllowed()) return;
  cfgUpdate([](Config& c) { memset(c.tcal, 0, sizeof c.tcal); });
  UiEvent e;
  e.kind = EventKind::StartTouchCal;
  postUiEvent(e);
  sendJson(200, "{\"status\":\"ok\"}");
}

void handleStatus() {
  NetStatus n;
  GpsState g;
  g_store.net.read([&](const NetStatus& v) { n = v; });
  g_store.gps.read([&](const GpsState& v) { g = v; });
  JsonDocument d;
  d["ip"] = n.ip;
  d["rssi"] = n.rssi;
  d["uptime_s"] = millis() / 1000;
  d["heap_free"] = ESP.getFreeHeap();
  d["version"] = FW_VERSION;
  d["gps"]["active"] = g.active;
  d["gps"]["fix"] = g.fix;
  d["gps"]["lat"] = g.lat;
  d["gps"]["lon"] = g.lon;
  d["qrz_status"] = qrzStatusStr();
  d["uart2_rx_pin"] = PIN_UART2_RX;
  d["uart2_tx_pin"] = PIN_UART2_TX;
  sendDoc(200, d);
}

void handleUartStatus() {
  Config c;
  cfgGet(c);
  GpsState g;
  g_store.gps.read([&](const GpsState& v) { g = v; });
  JsonDocument d;
  d["uart2_connected_module"] = c.uart2_cmod;
  d["gps_active"] = g.active;
  d["gps_fix"] = g.fix;
  d["bt_active"] = uart2BridgeEnabled();
  d["aprs_uart_active"] = !strncmp(c.uart2_cmod, "aprs_tnc", 8);
  sendDoc(200, d);
}

void handleBridgeStatus() {
  JsonDocument d;
  uart2BridgeStatus(d);
  sendDoc(200, d);
}

void handleBridgeScan() {
  if (!postAllowed()) return;
  if (!uart2BridgeEnabled()) return sendJson(400, "{\"status\":\"error\",\"message\":\"bridge_mode_required\"}");
  if (!uart2BridgeScan()) return sendJson(503, "{\"status\":\"error\",\"message\":\"busy\"}");
  sendJson(200, "{\"status\":\"ok\",\"action\":\"scan_started\"}");
}

void handleBridgePair() {
  if (!postAllowed()) return;
  if (!uart2BridgeEnabled()) return sendJson(400, "{\"status\":\"error\",\"message\":\"bridge_mode_required\"}");
  JsonDocument d;
  if (deserializeJson(d, server.arg("plain").c_str())) return sendJson(400, "{\"status\":\"error\",\"message\":\"json_parse\"}");
  const char* pw = d["password"] | "";
  if (strlen(pw) > 16) return sendJson(400, "{\"status\":\"error\",\"message\":\"password_too_long\"}");
  if (!uart2BridgePair(d["name"] | "", d["address"] | "", pw))
    return sendJson(400, "{\"status\":\"error\",\"message\":\"pair_start_failed\"}");
  sendJson(200, "{\"status\":\"ok\",\"action\":\"pair_started\"}");
}
}  // namespace

void webInit() {
  static const char* kHeaders[] = {"Content-Type", "Origin"};  // WebServer only keeps headers it was told to collect
  server.collectHeaders(kHeaders, 2);
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/config", HTTP_GET, handleConfig);
  server.on("/api/save", HTTP_POST, handleSave);
  server.on("/api/reset_touch", HTTP_POST, handleResetTouch);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/uart_status", HTTP_GET, handleUartStatus);
  server.on("/api/bridge_bt_status", HTTP_GET, handleBridgeStatus);
  server.on("/api/bridge_bt_scan", HTTP_POST, handleBridgeScan);
  server.on("/api/bridge_bt_pair", HTTP_POST, handleBridgePair);
}

void webTick() {
  if (!started) {
    // The TCP/IP stack only exists once wifiTick has brought up STA or AP.
    bool up = false;
    g_store.net.read([&](const NetStatus& n) { up = n.sta || n.ap; });
    if (!up) return;
    server.begin();
    started = true;
  }
  server.handleClient();
  if (restartAt && (int32_t)(millis() - restartAt) >= 0) ESP.restart();
}
