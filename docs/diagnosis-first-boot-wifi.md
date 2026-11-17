# Diagnosis: WiFi join after the first boot following a flash

## Symptom
After `pio run -e cyd -t upload` the first boot sometimes logged `[WIFI] connecting to CHM` and never joined within the
40 s window, then fell back to the AP. A later plain reset joined in 4-5 s.

## What was tested (board on /dev/ttyUSB0, serial captured right after esptool's hard reset, DTR/RTS low)
- Unmodified firmware, 4 flash cycles: joined every time, 5.1-5.3 s after boot. Not reproduced.
- Same firmware plus event logging, 3 flash cycles: joined every time (4.7-5.4 s).
- (a) RF calibration, (c) hostname order, (g) init load, (h) power: the failure did not reproduce, so none could be
  tied to it. Boot reason after upload is `POWERON_RESET`, the same path as the working boots.

## Finding
Even on successful boots the first auth attempt is frequently dropped by the router:

    [WIFI] connecting to CHM            (t=1.1 s)
    [WIFI] t=2978 ev=5 reason=2         STA_DISCONNECTED, reason 2 = AUTH_EXPIRE
    [WIFI] t=3184 ev=4                  STA_CONNECTED (driver retried internally)
    [WIFI] t=4886 ev=7                  GOT_IP

Because `setAutoReconnect(false)` is set and the state machine issued a single `WiFi.begin()` for the whole 40 s, a
dropped auth/assoc exchange that the driver does not retry leaves the STA idle until the timeout. This matches the
reported log (no event, no `connected`, AP at ~41 s). Root cause: no retry of an unanswered join attempt; the trigger
(router ignoring the first auth after the previous association) is outside our code and was not reproducible on demand
in 7 cycles, so the failing path itself was not captured.

## Fix
`wifi_manager.cpp` re-issues `WiFi.disconnect()` + `WiFi.begin()` every 10 s inside the 40 s window, and logs the
disconnect reason code. The mechanism was verified on the board with the interval temporarily set to 2 s (retry fired,
joined on the second attempt; a 200 ms delay after `disconnect()` is needed or `begin()` returns "sta is connecting").
