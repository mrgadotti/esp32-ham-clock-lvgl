#pragma once
// Pure protocol + state machine for the ESP32 BT bridge module on UART2. No I/O: the caller feeds reply
// lines to onLine(), calls tick(nowMs) periodically and writes whatever popCommand() returns (+ "\r\n").
#include <stdint.h>
#include <stddef.h>

#define BRIDGE_MAX_DEVICES 24
#define BRIDGE_CMD_MAX 96

enum BridgeEvent {
  BR_NONE, BR_ACK_BTSPP_OFF, BR_ACK_BTSPP_ON, BR_ACK_BT_SCAN, BR_AUTO_CONNECT_OK, BR_DISC_OK, BR_DISC_ERR,
  BR_LIST_BEGIN, BR_NAME, BR_LIST_END, BR_ERROR, BR_CNT_OK, BR_CNT_ERR
};

BridgeEvent bridgeParseLine(const char* line);
// "BT[NAME]:<name>[ADR]<addr>[BT]" -> sanitized name and 12-hex upper-case address.
bool bridgeParseNameLine(const char* line, char* name, size_t nameCap, char* addr12 /*13 bytes*/);

struct BridgeDevice { char name[41]; char addr[13]; };

class BridgeFsm {
public:
  BridgeDevice devices[BRIDGE_MAX_DEVICES];
  int deviceCount = 0;
  bool scanInProgress = false;
  int pairResult = 0;                      // 0 none/pending, 1 ok, -1 error
  char lastAck[24] = "", lastError[40] = "", lastDisc[16] = "";
  char pairedAddr[13] = "", pairedName[41] = "";
  bool autoConnectPending = false;         // pair with the saved device when the scan list ends

  void onLine(const char* line);
  void tick(uint32_t nowMs);               // call before popCommand(): it supplies the clock for cooldown/retries
  bool popCommand(char* out, size_t cap);  // false if empty or inside the 250 ms command cooldown
  void startScan();                        // BTSPP_OFF -> (ACK) -> BT SCAN
  bool pair(const char* name, const char* addr, const char* password);

private:
  char q_[4][BRIDGE_CMD_MAX];
  int qn_ = 0;
  uint32_t now_ = 0, lastTx_ = 0, retryAt_ = 0, gpsOnAt_ = 0;
  bool hasTx_ = false, awaitScanOffAck_ = false, awaitOnAck_ = false, gpsOnSent_ = false, btsppOnSent_ = false;
  uint8_t retries_ = 0;
  char pendAddr_[13] = "", pendName_[41] = "";
  bool push(const char* cmd);
  void serviceRetry();
};
