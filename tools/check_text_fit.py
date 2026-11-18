#!/usr/bin/env python3
"""Host-side text fit audit for the 320x240 UI.

Measures worst-case strings (every EN and PT-BR tr() string plus worst-case data) with the real glyph advance
widths parsed from src/ui/fonts/font_*.c (adv_w in 1/16 px, rounded per glyph like LVGL; kerning ignored, so
results can differ by about 1 px per pair) and compares them with the width each layout gives the label.

The AVAIL_* constants below mirror the layout code (file named next to each one); keep them in sync when a
layout changes. Usage: python3 tools/check_text_fit.py   (exit status 1 when anything overflows)
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FONTS = {}


def load(name):
    src = open(f"{ROOT}/src/ui/fonts/{name}.c", encoding="utf-8").read()
    dsc = src[src.index("glyph_dsc[] = {"):]
    dsc = dsc[:dsc.index("};")]
    adv = [int(m) for m in re.findall(r"\.adv_w = (\d+)", dsc)]
    cm = [(int(a), int(b), int(c)) for a, b, c in re.findall(
        r"\.range_start = (\d+), \.range_length = (\d+), \.glyph_id_start = (\d+)", src)]
    m = re.search(r"glyph_id_ofs_list_0\[\] = \{([^}]*)\}", src)  # FORMAT0_FULL (symbol-only fonts)
    if m:
        ofs = [int(x) for x in m.group(1).replace("\n", " ").split(",") if x.strip()]
        cm = [(cm[0][0], cm[0][1], cm[0][2], ofs)]
    cm = [c if len(c) == 4 else c + (None,) for c in cm]
    FONTS[name] = (adv, cm)


for n in ("font_14", "font_18", "font_24", "font_clock_48"):
    load(n)


def width(font, s):
    """Pixel width of the widest line of s."""
    adv, cm = FONTS[font]
    best = 0
    for line in s.split("\n"):
        w = 0
        for ch in line:
            cp = ord(ch)
            for st, ln, gid, ofs in cm:
                if st <= cp < st + ln:
                    w += (adv[gid + (ofs[cp - st] if ofs else cp - st)] + 8) // 16
                    break
            else:
                raise SystemExit(f"glyph U+{cp:04X} {ch!r} missing in {font}: {s!r}")
        best = max(best, w)
    return best


h = open(f"{ROOT}/lib/core/i18n.h", encoding="utf-8").read()
TR = {k: (en, pt) for k, en, pt in re.findall(r'X\((\w+), "((?:[^"\\]|\\.)*)", "((?:[^"\\]|\\.)*)"\)', h)}

rows = []  # (screen, item, font, avail, strings)


def add(screen, item, font, avail, strs):
    rows.append((screen, item, font, avail, strs))


def T(*keys):
    return [s for k in keys for s in TR[k]]


def both(fmt, *keys):
    """fmt applied to the EN and the PT variant of each key."""
    return [fmt % tuple(TR[k][i] for k in keys) for i in (0, 1)]


# ---------------- header (header.cpp: title at x=6 font_18, clock right-aligned at -6) ----------------
CLK = width("font_18", "00:00 Z")
HDR_FREE = 320 - 6 - 6 - 6  # title x=6, clock margin 6, 6 px gap
titles = ["Propagation", "HfBands", "Weather", "Forecast", "Settings", "AprsRadar", "DxCluster", "Pota", "HamAlert", "WeatherOm", "ForecastOm"]
add("Header", "title vs clock 'hh:mm Z'", "font_18", HDR_FREE - CLK, T(*titles))
add("Header", "APRS list title (x=46) vs clock", "font_18", HDR_FREE - 40 - CLK, T("AprsIs", "AprsKiss", "AprsTnc2"))
add("Header", "DX/POTA/HamAlert title (x=46) vs clock", "font_18", HDR_FREE - 40 - CLK, T("DxCluster", "Pota", "HamAlert"))
add("Header", "radar title vs 'x10' zoom", "font_18", HDR_FREE - width("font_18", "×10"), T("AprsRadar"))

# ---------------- spot tables (spot_table.cpp: cell pad_left SPOT_TABLE_PAD_L = 1, pad_right 0, font_14) ----------------
PAD = 1
SPOT_W = [42, 76, 60, 42, 100]  # spot_list_screen.cpp: UTC CALL MHz MOD COUNTRY|MSG (must sum to 320)
assert sum(SPOT_W) == 320
add("Spot list", "col UTC cell '23:59'", "font_14", SPOT_W[0] - PAD, ["23:59", "--"])
add("Spot list", "col CALL cell (8 chars; longer portable calls are cropped by design)", "font_14", SPOT_W[1] - PAD, ["PY2XXX/P", "9A1ABC/P"])
add("Spot list", "col MHz cell", "font_14", SPOT_W[2] - PAD, ["144.800", "430.000", "14.074"])
add("Spot list", "col MOD cell", "font_14", SPOT_W[3] - PAD, ["CW", "SSB", "DIGI", "-"])
add("Spot list", "col COUNTRY cell (cut at 15 chars)", "font_14", SPOT_W[4] - PAD, ["United States", "Brazil", "Germany"])
for i, k in enumerate(["Utc", "ColCall", "Mhz", "ColMod"]):
    if i == 0: add("Spot list", "col header LOC", "font_14", SPOT_W[0] - PAD, T("ColLocal"))
    add("Spot list", f"col header {k}", "font_14", SPOT_W[i] - PAD, T(k))
add("Spot list", "col header COUNTRY / MSG", "font_14", SPOT_W[4] - PAD, T("ColCountry", "ColMsg"))
# the waiting label is 300 px wide and wraps (spot_table.cpp), so two lines are available
add("Spot list", "waiting / no-login labels (font_18, wrapped, 2 lines)", "font_18", 600, T("WaitingForSpots", "FetchFailedRetrying", "HamAlertNoLogin", "WaitingForAprs"))

# ---------------- APRS list (aprs_list_screen.cpp: table cols, same cell padding) ----------------
APRS_W = [41, 93, 43, 58, 85]  # UTC CALL KM FREQ MSG (must sum to 320)
assert sum(APRS_W) == 320
add("APRS list", "col UTC cell '23:59'", "font_14", APRS_W[0] - PAD, ["23:59", "--"])
add("APRS list", "col CALL cell 'PY2XXX-15'", "font_14", APRS_W[1] - PAD, ["PY2XXX-15", "WA1ABC-15", "PU2MMM-10"])
add("APRS list", "col KM cell '9999' (>= 10000 km shows '12k')", "font_14", APRS_W[2] - PAD, ["9999", "12k", "-"])
add("APRS list", "col FREQ cell '144.800'", "font_14", APRS_W[3] - PAD, ["144.800", "430.000"])
add("APRS list", "col header LOC", "font_14", APRS_W[0] - PAD, T("ColLocal"))
for i, k in enumerate(["Utc", "ColCall", "ColKm", "ColFreq", "ColMsg"]):
    add("APRS list", f"col header {k}", "font_14", APRS_W[i] - PAD, T(k))
add("APRS list", "menu sort tile (92 px)", "font_14", 92 - 6, T("SortTime", "SortCall", "SortDist"))
add("APRS list", "menu clear tile (148 px)", "font_14", 148 - 6, T("BeaconNow", "ClearAprsList"))
add("APRS list", "menu OK/CANCEL tile (100 px)", "font_14", 100 - 6, T("Ok", "Cancel"))
add("APRS list", "menu section labels", "font_14", 304, T("SortBy", "AprsSetting"))
add("APRS list", "menu checkbox labels", "font_14", 320 - 24 - 28, T("Beaconing", "AprsAlert", "RangeAlert", "LedBuzzer"))
add("APRS list", "waiting label (font_18, centred)", "font_18", 320, T("WaitingForAprs"))

# ---------------- filter menu (filter_menu.cpp) ----------------
add("Filter menu", "title font_18", "font_18", 304, T("Filters"))
add("Filter menu", "section labels", "font_14", 304, T("Mode", "Band"))
add("Filter menu", "mode tile (70 px)", "font_14", 70 - 6, T("All", "FilterCw", "FilterSsb", "FilterDigi"))
add("Filter menu", "band tile (72 px)", "font_14", 72 - 6, ["160m", "Other", "UHF", "ALL", "TODOS"])
add("Filter menu", "DX clear tile (148 px)", "font_14", 148 - 6, T("ClearDxList"))
add("Filter menu", "OK/CANCEL tile (100 px)", "font_14", 100 - 6, T("Ok", "Cancel"))

# ---------------- APRS radar (aprs_radar_screen.cpp: text boxes 60 px / 80 px centred) ----------------
add("APRS radar", "range label '1000km' (60 px box)", "font_14", 60, ["1000km", "99.9km", "5.00km"])
add("APRS radar", "callsign label (measured, centred; collisions skipped)", "font_14", 120, ["PY2XXX-15", "PU2MMM-10"])
add("APRS radar", "reset button '1x' (44 px)", "font_14", 44 - 4, ["1×"])

# ---------------- Propagation (propagation_screen.cpp) ----------------
add("Propagation", "row labels (x=10 .. value box x=110)", "font_14", 110 - 10 - 4, T("Sfi", "KIndex", "AIndex", "Muf"))
add("Propagation", "values font_24 (200 px box)", "font_24", 200, ["999", "9", "99.9 MHz", "No data", "--"])
add("Propagation", "UPDATED UTC label", "font_14", 300, T("UpdatedUtc"))
add("Propagation", "updated stamp (24 chars)", "font_14", 300, ["16 Nov 2026 1200 GMT", "16 Nov 2026 12:00 GMT+"])
add("Propagation", "age 'updated 999 min ago'", "font_14", 300, [s.replace("%d", "999") for s in T("UpdatedMinAgo")])
add("Propagation", "No data error", "font_14", 300, T("NoData"))

# ---------------- HF bands (bands_screen.cpp: columns x=8,150,240) ----------------
add("HF bands", "col BAND label/value (142 px)", "font_14", 150 - 8 - 4, T("Band") + ["80m-40m", "12m-10m"])
add("HF bands", "col DAY header+values (90 px)", "font_14", 240 - 150 - 4, T("ColDay", "Good", "Fair", "Poor") + ["--"])
add("HF bands", "col NIGHT header+values (80 px)", "font_14", 320 - 240 - 4, T("ColNight", "Good", "Fair", "Poor") + ["--"])
add("HF bands", "UPDATED UTC label (x=10, err at x=150)", "font_14", 150 - 10 - 4, T("UpdatedUtc"))
add("HF bands", "No data error (x=150)", "font_14", 320 - 150 - 4, T("NoData"))
add("HF bands", "age text", "font_14", 300, [s.replace("%d", "999") for s in T("UpdatedMinAgo")])

# ---------------- Weather (weather_screen.cpp) ----------------
add("Weather", "TEMPERATURE label (x=15, icon x=215)", "font_14", 215 - 15 - 4, T("Temperature"))
add("Weather", "temp value font_24", "font_24", 215 - 15 - 4, ["-12.5 °C", "-40.0 °C"])
add("Weather", "HUMIDITY/PRESSURE/WIND labels (x=15, value x=110)", "font_14", 110 - 15 - 4, T("Humidity", "Pressure", "Wind"))
add("Weather", "values font_18 (x=110)", "font_18", 320 - 110 - 4,
    ["100%", "1013 hPa", "1013 hPa", "100.0 m/s", "100.0 km/h"])
add("Weather", "PM2.5 (x=15 .. PM10 x=165)", "font_18", 165 - 15 - 4, ["PM2.5: 999.9"])
add("Weather", "PM10 (x=165)", "font_18", 320 - 165 - 4, ["PM10: 999.9"])
add("Weather", "description (160 px, 2 lines DOT)", "font_14", 160 * 2 - 12, ["thunderstorm with heavy drizzle"])
FOOT = ["%s%s  (999 %s)" % (TR["WeatherForLocation"][i], "S\u00e3o Paulo", TR["Min"][i]) for i in (0, 1)]
add("Weather", "footer (320 px)", "font_14", 320 - 4, FOOT + ["Open-Meteo.com  (999 %s)" % TR["Min"][i] for i in (0, 1)] + T("NoApiKey", "WaitingForData", "NoData", "ForecastNoData"))

# ---------------- Forecast (forecast_screen.cpp: 4 columns of 79 px) ----------------
add("Forecast", "col head font_18 (79 px)", "font_18", 79 - 2, T("Plus3h", "Plus6h", "DaySun", "DayMon", "DayTue", "DayWed", "DayThu", "DayFri", "DaySat") + ["D+3"])
add("Forecast", "col temp font_18", "font_18", 79 - 2, ["-12°", "--"])
add("Forecast", "col night 'NIGHT -88' (no degree sign: the temp row above has it)", "font_14", 79 - 2, both("%s -88", "ColNight"))
add("Forecast", "col humidity font_18", "font_18", 79 - 2, ["100%"])
add("Forecast", "col wind '12.5m/s'", "font_14", 79 - 2, ["99.9m/s"])
add("Forecast", "message (296 px, wraps to 2 lines)", "font_18", 296 * 2, T("NoApiKey", "ForecastNoData", "NoData", "NoForecast"))

# ---------------- Clock (clock_screen.cpp: frame 300 px, labels 320 px centred) ----------------
add("Clock", "callsign font_24", "font_24", 296, ["PY2XXX-15/QRP"])
add("Clock", "OPERATOR STATION", "font_14", 296, T("OperatorStation"))
add("Clock", "time font_clock_48 (224 px label, per-second redraw)", "font_clock_48", 224, ["00:00:00", "88:88:88"])
add("Clock", "mode LOCAL TIME / UTC", "font_14", 296, T("LocalTime", "Utc"))
add("Clock", "date 'Weekday, 30 Month 2026' font_18 (3-letter weekday when the full text is wider than 296 px)", "font_18", 296,
    [(lambda full, abbr: full if width("font_18", full) <= 296 else abbr)(
        "%s, 30 %s 2026" % (TR[w][i], TR[m][i]), "%s, 30 %s 2026" % (TR["Day" + w[7:10]][i], TR[m][i]))
     for i in (0, 1) for w in TR if w.startswith("Weekday") for m in TR if m.startswith("Month")])
add("Clock", "IP / NO CONNECTION (centred)", "font_18", 296, ["255.255.255.255"] + T("NoConnection"))

# ---------------- Settings (settings_screen.cpp: row 308 px; dropdown/slider widgets 120+) ----------------
add("Settings", "label beside slider (308-152)", "font_14", 308 - 152 - 4, T("Brightness"))
add("Settings", "label beside switch (308-60)", "font_14", 308 - 60 - 4, T("NightDim", "InvertColors"))
add("Settings", "label beside dropdown (308-120)", "font_14", 308 - 120 - 4, T("Rotation", "Language", "AutoSwitch", "AlertTime", "AprsRadius"))
add("Settings", "dropdown option text (120 px - arrow)", "font_14", 120 - 40, ["Português", "300 s", "English", "100 km"])
add("Settings", "Calibrate touch button (max 150 px - 16 pad)", "font_14", 150 - 16, T("CalibrateTouch"))
add("Settings", "title right of back arrow (x=44) vs end of bar", "font_18", 320 - 44 - 6, T("Settings"))
add("Settings", "section heading", "font_14", 300, T("EnabledScreens"))
add("Settings", "screen checkbox names", "font_14", 300 - 30, T("Clock", "DxCluster", "Propagation", "HfBands", "Weather", "AprsIs", "Pota", "HamAlert", "AprsRadar", "Forecast", "WeatherOm", "ForecastOm"))
add("Settings", "info rows (300 px DOT)", "font_14", 300, ["SSID: " + "x" * 12, "IP: 255.255.255.255", "RSSI: -100 dBm", "%s: 999999 B" % TR["FreeHeap"][1], "%s: Nov 16 2026 12:00:00" % TR["Firmware"][1]])

# ---------------- Boot (boot_screen.cpp) ----------------
add("Boot", "title font_24", "font_24", 320, ["ESP32-HAM-CLOCK"])
add("Boot", "callsign font_24", "font_24", 320, ["PY2XXX-15/QRP"])
add("Boot", "log lines (x=10, ellipsis if longer; 39 chars max)", "font_14", 300, [
    "ESP32-HAM-CLOCK 2.0.0-dev", "Config: PY2XXX-15/QRP, PT-BR", "Display 320x240 rot 3 inv 1", "Touch calibration needed",
    "WiFi connecting to MyHomeNetwork-5G", "WiFi 255.255.255.255 -100 dBm", "AP fallback 192.168.4.1",
    "NTP synced 2026-11-16 12:00 UTC", "DNS fallback 1.1.1.1 / 8.8.8.8", "Svc: POTA PROP DX APRS HAMA WX QRZ",
    "DX connected dxc.ve7cc.net:23", "Heap free 123456 max 123456"])

# ---------------- Touch calibration (touch_cal_ui.cpp, centred label) ----------------
add("Touch cal", "prompts (320 px)", "font_14", 320, T("CalTouch", "CalVerify", "CalBad", "CalFailed"))

# ---------------- Overlays (alert_overlay.cpp) ----------------
add("Alert overlay", "APRS title y=8 (X button x>=272, centred)", "font_24", 320 - 2 * 48, T("AprsAlertTitle"))
add("Alert overlay", "beacon/weather title y=42 (below X button)", "font_24", 316, T("AprsBeaconing", "WeatherAlert"))
add("Alert overlay", "APRS call font_24 (x=18 .. compass x=236)", "font_24", 236 - 18 - 6, ["PY2XXX-15"])
add("Alert overlay", "APRS freq line font_14", "font_14", 236 - 18 - 6, ["144.800 MHz"])
add("Alert overlay", "APRS dist box (80 px)", "font_14", 80, ["99999 km", "- km"])
add("Alert overlay", "APRS symbol description font_18 (x=18 .. 226)", "font_18", 226 - 18 - 4,
    [TR[k][i] for k in TR if k.startswith("Desc") for i in (0, 1)])
WXL = []
for i in (0, 1):
    p = lambda k: TR[k][i]
    WXL += ["%s -12.5 C" % p("WxTemp"), "%s 100 %%" % p("HumidityLabel"), "%s 1013.2 hPa" % p("PressureLabel"),
            "%s 359° 123.4 km/h" % p("WxWind")]
add("Alert overlay", "APRS wx lines font_14 (x=18 .. compass 226)", "font_14", 226 - 18 - 4, WXL)
add("Alert overlay", "APRS comment (284 px, 2 lines)", "font_14", 284 * 2, ["Lorem ipsum dolor sit amet consectetur adipisci"])
add("Alert overlay", "weather alert body (226 px beside icon)", "font_18", 226,
    T("PossibleStorm", "StormNow", "StrongWind") + both("%s 99.9 m/s", "WindLabel") + both("%s 99.9 m/s", "Gust")
    + [s + " 999 min" for s in T("Repeat")])
add("Alert overlay", "beacon body (290 px; call[12], '%.4f %.4f' position, path of aprs_beacon.cpp)", "font_18", 290,
    ["PY2XXX-15/QRP", "-90.0000 -180.0000", "APRS,WIDE1-1,WIDE2-1", "APRS,TCPIP*"])
add("Alert overlay", "APRS bearing box (60 px)", "font_14", 60, ["359°", "---"])
add("Alert overlay", "APRS dist box 'N km' (80 px)", "font_14", 80, both("99999 %s", "Km"))

# ---------------- report ----------------
bad = warn = 0  # WARN: fits, but within 1 px of the limit (kerning is ignored, so the real width may differ by ~1 px)
cur = None
for screen, item, font, avail, strs in rows:
    worst = max(strs, key=lambda s: width(font, s))
    w = width(font, worst)
    over = w > avail
    tight = not over and w >= avail - 1
    bad += over
    warn += tight
    if screen != cur:
        print(f"\n== {screen}")
        cur = screen
    print(f"  {'OVERFLOW' if over else 'WARN' if tight else 'ok':8} {item:52} {font:13} need {w:4} / {avail:4}  {worst!r}")
print(f"\n{bad} overflow(s), {warn} tight (<= 1 px slack) in {len(rows)} checks")
sys.exit(1 if bad else 0)
