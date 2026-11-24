# APRS Messages Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** When an APRS message addressed to this station arrives, show an alert overlay on any screen (sender icon, sender call, distance, text), keep the last 20 messages, and add a Messages screen (opened from the APRS list menu) whose rows re-open the alert.

**Architecture:** A pure parser in `lib/core` turns a TNC2 line into an `AprsMessage` (native-tested). `aprsIngestLine` checks for a message first, enriches it from the stations ring (symbol, distance), stores it in a new `Guarded` ring and posts a new `EventKind::AprsMessage`; `alert_overlay.cpp` renders it with the same frame, timeout and queue as the other APRS alerts. The Messages list is a modal panel on `lv_layer_top` (same pattern as `filter_menu`), stacked above the APRS menu.

**Tech Stack:** PlatformIO espressif32@6.12.0, Arduino, LVGL 9.6.0, Unity native tests (`pio test -e native`).

**Spec:** none on disk. The user's request of 2026-11-24 is the spec, summarized under "Requirements".

## Execution roles (requested by the user)

- **Implementation tasks (0-5, code and tests): a fresh Sonnet 5.5 subagent per task** (`model: "sonnet"`), several running in parallel per wave (below). Each Sonnet agent edits only the files listed in its task and does not run `pio` or `git`.
- **Build, test-run and git steps (every "Run", "Build", "Commit" step, and Task 6): a Haiku 5.5 subagent** (`model: "haiku"`), run once per wave, never in parallel with itself (parallel `pio` runs would collide in `.pio/`). It runs the commands given, reports the output verbatim, and commits each task separately with `git add <that task's files>` and the message given. No AI co-author trailer (global rule).

**Waves** (tasks in a wave touch disjoint files, so their Sonnet agents run in parallel):

| Wave | Sonnet agents (parallel) | Then Haiku |
|---|---|---|
| A | Task 0 (shared interfaces) | build `cyd` + `native`, commit |
| B | Task 1 (parser), Task 3 (overlay), Task 4 (list panel) | native tests, build `cyd`, one commit per task |
| C | Task 2 (ingest, filter, harness), Task 5 (APRS menu button) | build `cyd` + `cyd_test`, harness checks, one commit per task |
| D | none | Task 6, then Task 7, then Task 8 (separate Haiku agents) |

If a wave's build fails, the Haiku agent reports the error and the owning task's Sonnet agent (continue it with SendMessage) fixes it before the next wave starts.
- **Task 7 (push and merge): a new, separate Haiku 5.5 subagent.**
- **Task 8 (upload to the board): another new, separate Haiku 5.5 subagent.**
- Tasks 7 and 8 start only after Task 6 passed. They are authorized by the user in this plan; nothing else in the plan pushes or merges.
- Commits are plain, English, no `Co-Authored-By`, no "Generated with" line.

## Requirements (from the user)

1. On receipt of an APRS message addressed to us, open an alert modal on top of whatever screen is active.
2. The alert shows the sender's icon, the sender, the text and the distance. It stays open `cfg().alrt_scr_s` seconds, like the other APRS alerts.
3. In the APRS list menu (the one with Sort by, Beacon now), add a **Messages** button below **Beacon now**.
4. Messages opens a new screen listing the stored messages. Tapping one re-opens the alert with that message.
5. Keep 10 messages, dropping the oldest; raise to 20 if RAM allows (measured in Task 6: 20 left only 4 KB of LV_MEM with the menu and list open, so the shipped value is 10).
6. The list rows are tall (easy to tap). Header: back arrow on the left (returns to the previous screen, the APRS menu), X on the right (returns to the APRS-IS screen).

## Global Constraints

- Code, comments, identifiers and commit messages in English. Strings go through `lib/core/i18n.h` `TR_LIST` with both EN and PT-BR.
- Fixed `char[]` structs only in `lib/core` (no `String`, no heap). `lib/core` must stay Arduino-free so `pio test -e native` builds it.
- `Guarded` store callbacks run under the lock: keep them short, never nest stores.
- LVGL 9.6.0 API (`lv_screen_*`, `lv_button_create`). `LV_MEM_SIZE` is 40 KB: free the panel's objects when it closes.
- UI objects are created and deleted on the UI task only. Delete panels with `lv_obj_delete_async`.
- Screen is 320x240 (`SCREEN_W`, `SCREEN_H`); `HEADER_H` is 26.
- Messages that are not addressed to us (bulletins, other calls) and `ack`/`rej` frames never alert and are never stored.

## Review Focus

1. Duplicate retries: senders resend an unacked message several times. Same from+msgNo+text must not alert or store again (Task 1 `aprsMsgSame`, Task 2).
2. Message from a station we have not heard a position for: icon is omitted, distance shows `- km`, nothing crashes (Task 2 and Task 3).
3. Addressee with SSID (`PY2XX-7`) vs plain call, lower-case on the wire, 9-char space padding (Task 1).
4. Alert queue is 4 deep: a burst of messages while an overlay shows must not lose the stored copies (Task 2: store first, post after).
5. APRS screen swapped out (auto-switch, language rebuild) while Messages is open must not leak or dangle the panel (Task 5 destructor).

---

### Task 0: Shared interfaces (Wave A)

**Files:**
- Modify: `lib/core/types.h`, `src/app/data_store.h`, `src/app/events.h`, `lib/core/i18n.h`

**Interfaces (all produced here, consumed by Tasks 1-5):**
- `struct AprsMessage { uint32_t utc; char from[12]; char text[68]; char msgNo[8]; char sym[2]; bool hasLatLon; int16_t distKm; };` in `types.h`. `sym` is the sender's last known symbol (two spaces if unknown), `distKm` is -1 if unknown.
- `constexpr size_t kAprsMessages = 20;`, `using AprsMsgRing = RingBuffer<AprsMessage, kAprsMessages>;` and `Guarded<AprsMsgRing> aprsMsg;` in `DataStore` (index 0 = newest). `DataStore` is heap-allocated, so the ~1.9 KB is not static DRAM.
- `EventKind::AprsMessage` appended to the enum, and `AprsMessage msg;` in the `UiEvent` union.
- i18n keys `Tr::AprsMessageTitle` ("APRS MESSAGE" / "MENSAGEM APRS") and `Tr::Messages` ("Messages" / "Mensagens").

- [ ] **Step 1: Add the four items above.**
- [ ] **Step 2: Run** `pio test -e native -f test_i18n` and `pio run -e cyd`. Expected: PASS and SUCCESS.
- [ ] **Step 3: Commit.** `git add lib/core/types.h lib/core/i18n.h src/app/data_store.h src/app/events.h && git commit -m "feat: add APRS message type, store, event and strings"`

---

### Task 1: Message parser (core, native tests) (Wave B)

**Files:**
- Create: `lib/core/aprs_message.h`, `lib/core/aprs_message.cpp`
- Test: `test/test_aprs_message/test_main.cpp`

**Interfaces:**
- Consumes: `AprsMessage` (Task 0). The parser leaves `utc`, `hasLatLon` zeroed and `sym` = two spaces, `distKm` = -1; the ingest step fills them.
- Produces:
  - `bool aprsParseMessage(const char* tnc2, const char* myCall, AprsMessage& out);` `myCall` is the login call with `-SSID` appended when the SSID is > 0 (e.g. `"PY2XX-7"`).
  - `bool aprsMsgSame(const AprsMessage& a, const AprsMessage& b);` true when `from`, `msgNo` and `text` are all equal.

- [ ] **Step 1: Write the failing tests** in `test/test_aprs_message/test_main.cpp` (style of `test/test_aprs_parse/test_main.cpp`):
  - `test_msg_for_me`: `"PU2ABC-9>APRS,TCPIP*::PY2XX-7  :Hello there{123"` with `myCall="PY2XX-7"` returns true; `from=="PU2ABC-9"`, `text=="Hello there"`, `msgNo=="123"`, `sym[0]==' '`, `distKm==-1`.
  - `test_msg_no_msgno`: `"A1B>APRS::PY2XX    :hi"` with `myCall="PY2XX"` returns true, `msgNo==""`.
  - `test_msg_case_insensitive`: wire `"a1b>aprs::py2xx    :hi"`, `myCall="PY2XX"` returns true, `from=="A1B"`.
  - `test_msg_not_for_me`: addressee `PY2XX-7` with `myCall="PY2XX"` returns false (SSID must match exactly); addressee `N0CALL` returns false; bulletin `"::BLN1     :text"` returns false.
  - `test_msg_ack_rej_ignored`: `":PY2XX    :ack123"` and `":PY2XX    :rej7"` return false; `":PY2XX    :acknowledged ok"` returns true (not an ack: contains a space).
  - `test_msg_empty_text`: `":PY2XX    :"` and `":PY2XX    :{12"` return false.
  - `test_msg_not_a_message`: a position frame `"A1B>APRS:!4903.50N/07201.75W-x"` returns false; null line and null `myCall` return false.
  - `test_msg_text_sanitized_truncated`: 80 chars of `'a'..` return `strlen(text)==67`; embedded `\r`, `\n` become spaces and bytes >= 0x80 are dropped.
  - `test_msg_same`: equal from+msgNo+text -> true; different msgNo -> false; different text -> false; different from -> false.

- [ ] **Step 2: Run to verify it fails.** Run: `pio test -e native -f test_aprs_message`. Expected: build FAIL, `aprs_message.h` not found.

- [ ] **Step 3: Implement** the two functions in `lib/core/aprs_message.cpp`. Format: after the first `':'` following the first `'>'`, the info field must start with `':'`, then a 9-char addressee padded with spaces, then `':'`, then text up to an optional `'{'` and a msgNo of 1..5 chars. Compare the addressee (trim trailing spaces) to `myCall` case-insensitively. Sanitize `from` like `aprsParseFrame` does (uppercase, keep `[A-Z0-9-]`) and the text like its `sanitizeComment` (printable ASCII, control whitespace to space, trimmed, capped at 67 chars); duplicate those few lines rather than exposing the static helper. An ack/rej is text that starts with `ack` or `rej` followed by 1..5 alphanumerics and nothing else.

- [ ] **Step 4: Run to verify it passes.** Run: `pio test -e native -f test_aprs_message`. Expected: all tests PASS.

- [ ] **Step 5: Commit.** `git add lib/core/aprs_message.h lib/core/aprs_message.cpp test/test_aprs_message && git commit -m "feat: parse APRS messages addressed to this station"`

---

### Task 2: Ingest, server filter and harness command (Wave C)

**Files:**
- Modify: `src/svc/aprs_ingest.cpp`, `src/svc/aprs_is.cpp` (`sendFilter`), `src/debug/screen_test.cpp` (harness command `msg`)

**Interfaces:**
- Consumes: `aprsParseMessage`, `aprsMsgSame` (Task 1), `g_store.aprs` (`AprsRing`), `g_store.aprsMsg`, `EventKind::AprsMessage` (Task 0).
- Produces: messages stored and posted as events; harness command `msg`.

- [ ] **Step 1: (moved to Task 0.)** Nothing to add here.

- [ ] **Step 2: In `aprs_ingest.cpp`** cache our call next to the other config values: inside `refreshRules`' `cfgRead` copy `aprs_callsign` and `aprs_ssid` into a file-static `myCall[16]` formatted `CALL` or `CALL-SSID`. This is the existing cached read (`cfgVersion()` gate), so it adds no per-line cost.

- [ ] **Step 3: In `aprsIngestLine`**, before `aprsParseFrame`, call `static bool ingestMessage(const char* line)` (new, same file). It copies `myCall` under `mu`, calls `aprsParseMessage`; on success it sets `utc` (same expression as for stations), looks the sender up in `g_store.aprs` by exact `call` to copy `sym`, `hasLatLon` and `distKm`, then under `g_store.aprsMsg.write` returns early if any stored entry is `aprsMsgSame` (duplicate), else `pushFront`. Only for a new message post `UiEvent{kind=AprsMessage, msg=...}` AFTER the store write. `aprsIngestLine` returns right after a handled message (position parser would reject it anyway). No LED blink (not requested).

- [ ] **Step 4: Server filter in `aprs_is.cpp`.** The current filter is `r/lat/lon/radius`. Per the APRS-IS docs it already passes messages addressed to stations inside the range (and the sender's position, even if the sender is outside), but only when the server knows our position, which normally needs us to beacon. To receive messages regardless, extend `sendFilter` so both the login and the later `#filter` update send `r/lat/lon/radius g/<CALL[-SSID]>` (`g/` = pass all message packets addressed to those callsign-SSIDs). Before coding, re-read the filter table at aprs-is.net/javAPRSFilter.aspx and use the exact syntax it states; if it differs, note it in the commit message. `fRadius`/move detection stays as is.

- [ ] **Step 5: Harness command** in `src/debug/screen_test.cpp`: `msg <FROM> <text...>` builds `"<FROM>>APRS::<CALL padded to 9>:<text>{<n>"` from `cfg().aprs_callsign`/`aprs_ssid` with an incrementing `n`, and calls `aprsIngestLine(line, 'I')`. Also `fill N` (line ~72) should be left alone.

- [ ] **Step 6: Build.** Run: `pio run -e cyd` and `pio run -e cyd_test`. Expected: both SUCCESS; paste the RAM/flash summary lines.

- [ ] **Step 7: Commit.** `git add src && git commit -m "feat: store APRS messages and raise a UI event"`

---

### Task 3: Alert overlay for messages (Wave B)

**Files:**
- Modify: `src/ui/overlays/alert_overlay.cpp`, `src/ui/overlays/alert_overlay.h` (comment only)

**Interfaces:**
- Consumes: `UiEvent` with `kind == EventKind::AprsMessage` and `Tr::AprsMessageTitle` (Task 0); `aprsSymbolInfo`, `aprsIconImage`, existing `aprsImage()` helper in the overlay file.
- Produces: the message alert. `overlayShow(const UiEvent&)` already works for this kind (`uiHandleEvent` default case), so no change in `ui_manager.cpp`.

- [ ] **Step 1: (moved to Task 0.)** Nothing to add here.
- [ ] **Step 2: (moved to Task 0.)** Nothing to add here.
- [ ] **Step 3: Extract the border frame** from `buildAprs` (lines creating `frame` and the 4 `edge` bars plus `setBorder(white, 3)`) into `void buildFrame(lv_obj_t* ov)` and call it from `buildAprs`, so the pulsing border is shared. No visual change to the existing alert.
- [ ] **Step 4: Implement `void buildMessage(lv_obj_t* ov, const AprsMessage& m)`** in the anonymous namespace and handle `case EventKind::AprsMessage` in `formatBody` (title `AprsMessageTitle`, `aprs = true`-style layout flag renamed or a second flag so the title is placed at y=8 like APRS; `secs` stays `cfg().alrt_scr_s`). Layout inside the 308x190 frame at (6,44): icon via `aprsImage(ik, 14, 50)` when `aprsSymbolInfo(m.sym, ik, d)` gives `ik != AprsIcon::None` (otherwise a `font_18` label with `tr(d)` only when `m.sym[0] != ' '`); sender call `font_24` white at x=82,y=52; distance `font_14` `COL_ACCENT` at x=82,y=82 as `"%d km"` when `m.hasLatLon && m.distKm >= 0`, else `"- km"` (use `tr(Tr::Km)`); message text `font_18` white at (18,112), width 284, `LV_LABEL_LONG_WRAP`, height 100 (67 chars wrap to at most 4 lines).
- [ ] **Step 5: Build.** Run: `pio run -e cyd`. Expected: SUCCESS.
- [ ] **Step 6: Visual check** (screen-test harness, `docs/port-notes` / `tools/screen_test.py`): flash `cyd_test`, send `msg PU2ABC-9 Hello from the test` on the serial port from screen 1 (clock) and from the APRS list, then `shot`. Expected: modal over both screens, border pulses, closes after `alrt_scr_s`; a long 67-char text stays inside the frame. Save the screenshot under `docs/review/screens/` as `aprs_msg_alert.png`.
- [ ] **Step 7: Commit.** `git add src/ui/overlays docs/review/screens/aprs_msg_alert.png && git commit -m "feat: show an alert overlay for APRS messages"`

---

### Task 4: Messages list panel

**Files:**
- Create: `src/ui/widgets/aprs_msg_list.h`, `src/ui/widgets/aprs_msg_list.cpp`

**Interfaces:**
- Consumes: `g_store.aprsMsg` (Task 2), `overlayShow` (Task 3), `uiModalPanel`, `uiScrollArea`, `uiLabel`, `headerCreate`-style accent bar, `formatHmCell`-style time (reuse what `aprs_list_screen.cpp` calls for the UTC column; read it first), `uiTimeOffsetSec()`.
- Produces:
  - `void aprsMsgListOpen(void (*onExit)());` opens the panel; `onExit` is called (once) when the X is tapped, after the panel is closed. Back arrow closes only the panel. No-op if already open.
  - `bool aprsMsgListIsOpen();`
  - `void aprsMsgListClose();` safe to call when not open.

- [ ] **Step 1: Implement `aprs_msg_list.cpp`.** Panel = `uiModalPanel()`. Header bar: 26 px accent strip at the top (same colours as `headerCreate`: `COL_ACCENT` bg, `COL_BG` text, `font_18`). Left: 50x26 clickable area with a back arrow drawn with three `lv_draw_line` calls in a `LV_EVENT_DRAW_MAIN` callback (pattern of `onCompassDraw` in `alert_overlay.cpp`; no font glyph is needed). Right: 50x26 clickable area with a drawn X (two lines). Centre/left-of-centre: title `tr(Tr::Messages)`. Below: `uiScrollArea(p, 0, HEADER_H, SCREEN_W, SCREEN_H - HEADER_H)`.
- [ ] **Step 2: Rows.** Snapshot the ring into a local `AprsMessage rows[kAprsMessages]` array under `g_store.aprsMsg.read` (heap-allocate it with `new`/`delete` inside `Panel`, not on the UI task stack: ~1.9 KB) and build one row per message, 48 px tall, 1 px gap: a clickable `lv_obj` (`COL_DIM`-ish dark background so rows are distinguishable) holding line 1 `"HH:MM  CALL"` (`font_14`, call in `COL_TEXT`, time in `COL_GREY`) and line 2 the text (`font_14`, `COL_CYAN`, fixed width 300, single line with the dots long mode; use the dots enum name that LVGL 9.6.0 defines, check `lv_label.h`). Row tap -> `overlayShow(UiEvent{kind=AprsMessage, msg=row})`. Empty ring: one centred `font_14` label `tr(Tr::NoData)`.
- [ ] **Step 3: Close handling.** Back and X handlers only set state and call `lv_obj_delete_async(panel)`, then free the `Panel` struct (same lifetime pattern as `filterMenuClose`). X additionally calls the saved `onExit`.
- [ ] **Step 4: Build.** Run: `pio run -e cyd` (the widget is unused until Task 5, this only catches compile errors). Expected: SUCCESS.
- [ ] **Step 5: Commit.** `git add src/ui/widgets/aprs_msg_list.* && git commit -m "feat: add the APRS messages list panel"`

---

### Task 5: Messages button in the APRS menu

**Files:**
- Modify: `src/ui/screens/aprs_list_screen.cpp` (menu layout at lines ~158-181, destructor, `closeMenu`)

**Interfaces:**
- Consumes: `aprsMsgListOpen`, `aprsMsgListClose` (Task 4), `Tr::Messages` (Task 3).

- [ ] **Step 1: Re-flow the menu** so Messages sits directly below Beacon now without moving OK/Cancel. Current rows: scroll area y=76 h=92, Beacon/Clear y=172, OK/Cancel y=208. New: scroll area y=76 **h=62** (rows still scroll), Beacon now / Clear APRS list at y=142, `Messages` tile at `x=8, y=174, w=148, h=26`, OK/Cancel unchanged at y=208. Paint the Messages tile with `uiTilePaint(..., true)` like its siblings.
- [ ] **Step 2: `onMessages` handler** (static, like `onBeacon`): it does NOT close the menu (the menu stays open underneath so unsaved sort/flag edits survive the back arrow). It calls `aprsMsgListOpen(onMessagesExit)`, where `onMessagesExit` calls `self_->closeMenu()` (X returns to the APRS-IS screen).
- [ ] **Step 3: Lifetime.** `closeMenu()` and `~AprsListScreen()` also call `aprsMsgListClose()`, so no panel is left on `lv_layer_top` if the screen is destroyed (auto-switch, language rebuild).
- [ ] **Step 4: Build.** Run: `pio run -e cyd`. Expected: SUCCESS.
- [ ] **Step 5: Interaction check** (harness, add a scenario to `tools/screen_test_interact.txt`: `goto 6`, `tap 35 13`, `msg` x3, tap Messages, `shot i_aprs_msgs`, tap a row, `shot i_aprs_msg_alert`, back arrow, `shot i_aprs_menu_back`, Messages again, X, `shot i_aprs_after_x`). Expected: list shows 3 rows newest first; row tap opens the alert on top of the list; back arrow shows the menu with Sort/flags unchanged; X shows the APRS list with no menu.
- [ ] **Step 6: Commit.** `git add src/ui/screens/aprs_list_screen.cpp tools/screen_test_interact.txt docs/review/screens && git commit -m "feat: add Messages button to the APRS menu"`

---

### Task 6: Full verification, RAM gate, docs (Haiku 5.5)

**Files:**
- Modify: `README.md` / `docs/api.md` only if they list APRS menu items or overlays (grep first).

- [ ] **Step 1: Native tests.** Run: `pio test -e native`. Expected: all suites PASS.
- [ ] **Step 2: Builds.** Run: `pio run -e cyd` and `pio run -e esp32-38pin`. Expected: SUCCESS; flash under the 3145728 B budget in `.github/workflows/build.yml`. Report RAM/flash numbers.
- [ ] **Step 3: RAM gate for 20 messages.** On `cyd_test`, `msg` 25 times (ring must hold 20, oldest dropped), open Messages from the APRS menu and run the harness `mem` command. Expected: `lvfree` (LV_MEM) stays >= 8 KB with the menu and list both open, and free heap does not drop by more than ~2 KB versus before the 25 messages. If not, change `kAprsMessages` to 10 in `data_store.h` and re-run Steps 1-2.
- [ ] **Step 4: Duplicate check.** Make the harness `msg` command reuse the msgNo when given a trailing `#<n>` argument (add this in Task 2 Step 5), then run `msg A1B hi #5` twice: one alert, one row.
- [ ] **Step 5: Commit** any doc change: `git commit -am "docs: mention APRS messages"`; skip if nothing changed. Do not push.

---

### Task 7: Push the branch and merge into main (new Haiku 5.5 agent)

**Files:** none (git only).

- [ ] **Step 1: Pre-checks.** Run: `git status --short` (expected: empty) and `git branch --show-current` (expected: `aprs-msg`). If the tree is dirty or Task 6 did not pass, stop and report.
- [ ] **Step 2: Push the branch.** Run: `git push -u origin aprs-msg`. Expected: branch published. Report the output.
- [ ] **Step 3: Merge into main.** Run: `git checkout main && git pull --ff-only origin main && git merge --no-ff aprs-msg -m "feat: APRS message alerts and messages list"`. Expected: merge commit, no conflicts. On conflict: `git merge --abort`, stop and report the conflicting files; do not resolve them.
- [ ] **Step 4: Re-test the merged tree.** Run: `pio test -e native` and `pio run -e cyd`. Expected: PASS and SUCCESS. If either fails, stop and report; do not push main.
- [ ] **Step 5: Push main.** Run: `git push origin main`. Expected: success. No force push, no `--no-verify`. Report the final `git log --oneline -3`.

---

### Task 8: Upload to the board (new Haiku 5.5 agent)

**Files:** none.

- [ ] **Step 1: Check the board.** Run: `pio device list`. Expected: one serial port for the CYD. If none is listed, stop and report (do not guess a port).
- [ ] **Step 2: Upload.** On `main` (after Task 7) run: `pio run -e cyd -t upload` (use the `pio` CLI via Bash, not the platformio MCP upload tool). Expected: `SUCCESS`. If the upload cannot connect, report the error text.
- [ ] **Step 3: Boot check.** Run: `pio device monitor -b 115200` for about 20 s (or read the port with a short timeout). Expected: normal boot log, no reset loop or panic. Report the first lines and any `[MEM]` lines.
- [ ] **Step 4: Report** what the user should test by hand: have another station send a message to the configured call and confirm the alert and the Messages list.

---

## Self-review

- **Spec coverage:** alert on any screen (Tasks 2-3), icon + sender + text + distance (Task 3), same duration (Task 3 uses `alrt_scr_s`), Messages button below Beacon now (Task 5), list screen with tap-to-reopen (Tasks 4-5), 10/20 cap with oldest dropped and RAM decision (Tasks 2, 6), tall rows, back arrow left and X right with the specified destinations (Tasks 4-5), Sonnet implementation and Haiku build/git (roles section).
- **Type consistency:** `AprsMessage`, `kAprsMessages`, `aprsParseMessage`, `aprsMsgSame`, `aprsMsgListOpen/IsOpen/Close`, `EventKind::AprsMessage`, `Tr::AprsMessageTitle`, `Tr::Messages` are used with the same names throughout.
- **Not included (on purpose):** sending ACKs back to the sender, LED blink on message, message replies, persistence across reboot (ring is RAM only), TNC/KISS-specific filtering (RF frames reach the same ingest path).
