#include "bridge_proto.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static const uint32_t COOLDOWN_MS = 250, RETRY_MS = 2000, FORCE_GPS_ON_MS = 60000, FORCE_BTSPP_DELAY_MS = 10000;
static const uint8_t MAX_TRIES = 3;

BridgeEvent bridgeParseLine(const char* l) {
  if (!l) return BR_NONE;
  if (strstr(l, "ACK BTSPP_OFF")) return BR_ACK_BTSPP_OFF;
  if (strstr(l, "ACK BTSPP_ON")) return BR_ACK_BTSPP_ON;
  if (strstr(l, "ACK BT SCAN")) return BR_ACK_BT_SCAN;
  if (strstr(l, "AUTO CONNECT OK")) return BR_AUTO_CONNECT_OK;
  if (strstr(l, "BTDISC_OK")) return BR_DISC_OK;
  if (strstr(l, "BTDISC_ERR")) return BR_DISC_ERR;
  if (strstr(l, "BT[LIST_BEGIN]:")) return BR_LIST_BEGIN;
  if (strstr(l, "BT[NAME]:")) return BR_NAME;
  if (strstr(l, "BT[LIST_END]")) return BR_LIST_END;
  if (strstr(l, "BT[ERROR]:")) return BR_ERROR;
  if (strstr(l, "BTCNT_OK")) return BR_CNT_OK;
  if (strstr(l, "BTCNT_ERR")) return BR_CNT_ERR;
  return BR_NONE;
}

// Printable ASCII only, no brackets/CR/LF, trimmed, at most maxLen chars.
static void sanitize(const char* in, size_t n, size_t maxLen, char* out) {
  size_t o = 0;
  for (size_t i = 0; i < n && in[i] && o < maxLen; i++) {
    unsigned char c = (unsigned char)in[i];
    if (c < 32 || c > 126 || c == '[' || c == ']') continue;
    out[o++] = (char)c;
  }
  out[o] = 0;
  while (o && out[o - 1] == ' ') out[--o] = 0;
  size_t s = 0;
  while (out[s] == ' ') s++;
  if (s) memmove(out, out + s, o - s + 1);
}

static bool normAddr(const char* in, char* out /*13*/) {
  size_t o = 0;
  for (; *in; in++) {
    if (*in == ':' || *in == '-' || *in == ' ') continue;
    if (!isxdigit((unsigned char)*in) || o >= 12) return false;
    out[o++] = (char)toupper((unsigned char)*in);
  }
  out[o] = 0;
  return o == 12;
}

bool bridgeParseNameLine(const char* line, char* name, size_t nameCap, char* addr12) {
  const char* p = line ? strstr(line, "BT[NAME]:") : nullptr;
  if (!p) return false;
  p += 9;
  const char* adr = strstr(p, "[ADR]");
  if (!adr) return false;
  const char* bt = strstr(adr + 5, "[BT]");
  if (!bt) return false;
  char rawAddr[24];
  size_t al = (size_t)(bt - (adr + 5));
  if (al >= sizeof rawAddr) return false;
  memcpy(rawAddr, adr + 5, al); rawAddr[al] = 0;
  if (!normAddr(rawAddr, addr12)) return false;
  char tmp[41];
  sanitize(p, (size_t)(adr - p), sizeof tmp - 1, tmp);
  if (!tmp[0]) strcpy(tmp, "BT_DEVICE");
  snprintf(name, nameCap, "%s", tmp);
  return true;
}

bool BridgeFsm::push(const char* cmd) {
  if (qn_ >= 4) return false;
  snprintf(q_[qn_++], BRIDGE_CMD_MAX, "%s", cmd);
  return true;
}

bool BridgeFsm::popCommand(char* out, size_t cap) {
  if (qn_ == 0) return false;
  if (hasTx_ && (uint32_t)(now_ - lastTx_) < COOLDOWN_MS) return false;
  snprintf(out, cap, "%s", q_[0]);
  memmove(q_[0], q_[1], (size_t)(qn_ - 1) * BRIDGE_CMD_MAX);
  qn_--;
  lastTx_ = now_; hasTx_ = true;
  return true;
}

void BridgeFsm::serviceRetry() {
  if (!awaitOnAck_ || (int32_t)(now_ - retryAt_) < 0) return;
  if (retries_ >= MAX_TRIES) {
    awaitOnAck_ = false;
    snprintf(lastError, sizeof lastError, "BTSPP_ON_NO_ACK");
    return;
  }
  if (!push("BTSPP_ON")) return;  // outbox full: try again next tick
  retries_++;
  retryAt_ = now_ + RETRY_MS;
}

void BridgeFsm::tick(uint32_t nowMs) {
  now_ = nowMs;
  serviceRetry();
  // Boot fallback: GPS ON at 60 s, BTSPP_ON 10 s later, regardless of connect status.
  if (!gpsOnSent_ && nowMs >= FORCE_GPS_ON_MS) {
    if (push("GPS ON")) { gpsOnSent_ = true; gpsOnAt_ = nowMs; }
  } else if (gpsOnSent_ && !btsppOnSent_ && (int32_t)(nowMs - (gpsOnAt_ + FORCE_BTSPP_DELAY_MS)) >= 0) {
    if (push("BTSPP_ON")) btsppOnSent_ = true;
  }
}

void BridgeFsm::startScan() {
  deviceCount = 0;
  scanInProgress = true;
  awaitScanOffAck_ = true;
  lastError[0] = lastDisc[0] = lastAck[0] = 0;
  push("BTSPP_OFF");
}

bool BridgeFsm::pair(const char* name, const char* addr, const char* password) {
  char a[13], n[41], pw[17];
  if (!normAddr(addr ? addr : "", a)) { snprintf(lastError, sizeof lastError, "PAIR_ADDR_INVALID"); return false; }
  sanitize(name ? name : "", 40, 40, n);
  if (!n[0]) strcpy(n, "BT_DEVICE");
  sanitize(password ? password : "", 16, 16, pw);
  char cmd[BRIDGE_CMD_MAX];
  int len = snprintf(cmd, sizeof cmd, "BTCNT[NAME]%s[ADR]%s", n, a);
  if (pw[0]) snprintf(cmd + len, sizeof cmd - (size_t)len, "[PSWD]%s", pw);  // PIN optional: "****" causes BTCNT_ERR
  push(cmd);
  strcpy(pendAddr_, a); strcpy(pendName_, n);
  pairResult = 0;
  return true;
}

void BridgeFsm::onLine(const char* line) {
  switch (bridgeParseLine(line)) {
    case BR_ACK_BTSPP_OFF:
      snprintf(lastAck, sizeof lastAck, "ACK BTSPP_OFF");
      if (awaitScanOffAck_) { awaitScanOffAck_ = false; push("BT SCAN"); }
      break;
    case BR_ACK_BTSPP_ON:
      snprintf(lastAck, sizeof lastAck, "ACK BTSPP_ON");
      lastError[0] = 0; awaitOnAck_ = false; retries_ = 0;
      break;
    case BR_ACK_BT_SCAN:
      snprintf(lastAck, sizeof lastAck, "ACK BT SCAN");
      scanInProgress = true; lastError[0] = 0;
      break;
    case BR_AUTO_CONNECT_OK:
    case BR_CNT_OK:
      if (bridgeParseLine(line) == BR_CNT_OK) {
        pairResult = 1;
        if (pendAddr_[0]) { strcpy(pairedAddr, pendAddr_); strcpy(pairedName, pendName_); }
        pendAddr_[0] = pendName_[0] = 0;
      } else {
        snprintf(lastAck, sizeof lastAck, "AUTO CONNECT OK"); lastError[0] = 0;
      }
      awaitOnAck_ = true; retries_ = 0; retryAt_ = 0;
      serviceRetry();
      break;
    case BR_DISC_OK: snprintf(lastDisc, sizeof lastDisc, "BTDISC_OK"); break;
    case BR_DISC_ERR: snprintf(lastDisc, sizeof lastDisc, "BTDISC_ERR"); break;
    case BR_LIST_BEGIN: deviceCount = 0; scanInProgress = true; break;
    case BR_NAME: {
      BridgeDevice d;
      scanInProgress = true;
      if (!bridgeParseNameLine(line, d.name, sizeof d.name, d.addr)) break;
      for (int i = 0; i < deviceCount; i++)
        if (!strcmp(devices[i].addr, d.addr)) { strcpy(devices[i].name, d.name); return; }
      if (deviceCount < BRIDGE_MAX_DEVICES) devices[deviceCount++] = d;
      break;
    }
    case BR_LIST_END:
      scanInProgress = false;
      if (autoConnectPending && strlen(pairedAddr) == 12) { pair(pairedName, pairedAddr, ""); autoConnectPending = false; }
      break;
    case BR_ERROR: {
      const char* p = strstr(line, "BT[ERROR]:") + 10;
      const char* end = strstr(p, "[BT]");
      sanitize(p, end ? (size_t)(end - p) : strlen(p), sizeof lastError - 1, lastError);
      scanInProgress = false;
      break;
    }
    case BR_CNT_ERR:
      pairResult = -1; snprintf(lastError, sizeof lastError, "BTCNT_ERR");
      pendAddr_[0] = pendName_[0] = 0;
      break;
    default: break;
  }
}
