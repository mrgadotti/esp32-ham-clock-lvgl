#include "web_config.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <WiFi.h>

#include "app/config_store.h"
#include "app/data_store.h"
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
const size_t kCfgBuf = 6 * 1024;

WebServer server(80);
bool started = false;
uint32_t restartAt = 0;  // millis() deadline, 0 = none

void sendJson(int code, const String& s) { server.send(code, "application/json", s); }

void handleRoot() {
  server.sendHeader("Content-Encoding", "gzip");
  server.send_P(200, "text/html", (PGM_P)CONFIG_HTML_GZ, CONFIG_HTML_GZ_LEN);
}

void handleConfig() {
  char* buf = (char*)malloc(kCfgBuf);
  if (!buf) return sendJson(503, "{\"error\":\"no_memory\"}");
  Config c;
  cfgGet(c);
  if (configToJson(c, buf, kCfgBuf)) sendJson(200, buf);
  else sendJson(500, "{\"error\":\"config_too_large\"}");
  free(buf);
}

void handleSave() {
  const String& body = server.arg("plain");
  if (body.length() > kMaxBody) return sendJson(413, "{\"error\":\"body_too_large\"}");
  ApplyResult r;
  cfgApplyJson(body.c_str(), r);
  if (!r.ok) {
    JsonDocument d;
    d["error"] = r.error;
    String s;
    serializeJson(d, s);
    return sendJson(400, s);
  }
  if (r.needsRestart) restartAt = (millis() + 1500) | 1;  // |1 keeps it nonzero
  sendJson(200, String("{\"status\":\"ok\",\"restart\":") + (r.needsRestart ? "true}" : "false}"));
}

void handleResetTouch() {
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
  String s;
  serializeJson(d, s);
  sendJson(200, s);
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
  String s;
  serializeJson(d, s);
  sendJson(200, s);
}

void handleBridgeStatus() {
  JsonDocument d;
  uart2BridgeStatus(d);
  String s;
  serializeJson(d, s);
  sendJson(200, s);
}

void handleBridgeScan() {
  if (!uart2BridgeEnabled()) return sendJson(400, "{\"status\":\"error\",\"message\":\"bridge_mode_required\"}");
  if (!uart2BridgeScan()) return sendJson(503, "{\"status\":\"error\",\"message\":\"busy\"}");
  sendJson(200, "{\"status\":\"ok\",\"action\":\"scan_started\"}");
}

void handleBridgePair() {
  if (!uart2BridgeEnabled()) return sendJson(400, "{\"status\":\"error\",\"message\":\"bridge_mode_required\"}");
  JsonDocument d;
  if (deserializeJson(d, server.arg("plain"))) return sendJson(400, "{\"status\":\"error\",\"message\":\"json_parse\"}");
  const char* pw = d["password"] | "";
  if (strlen(pw) > 16) return sendJson(400, "{\"status\":\"error\",\"message\":\"password_too_long\"}");
  if (!uart2BridgePair(d["name"] | "", d["address"] | "", pw))
    return sendJson(400, "{\"status\":\"error\",\"message\":\"pair_start_failed\"}");
  sendJson(200, "{\"status\":\"ok\",\"action\":\"pair_started\"}");
}
}  // namespace

void webInit() {
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
