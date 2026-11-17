#!/usr/bin/env bash
# Regenerates src/ui/fonts/*.c from tools/fonts/Montserrat-Medium.ttf (needs node). Outputs are committed.
# font_14 also carries the two LVGL symbols the theme draws (checkbox tick, dropdown arrow), taken from the
# FontAwesome file shipped inside the LVGL package (run `pio run` once so it is downloaded).
set -euo pipefail
cd "$(dirname "$0")/.."
TTF=tools/fonts/Montserrat-Medium.ttf
FA=${FA_WOFF:-.pio/libdeps/cyd/lvgl/scripts/generators/built_in_font/FontAwesome5-Solid+Brands+Regular.woff}
OUT=src/ui/fonts
mkdir -p "$OUT"
# ASCII, degree sign and every Latin-1 letter (PT-BR text, OpenWeather descriptions, city names). Only the
# punctuation block and U+00F7 are dropped; U+00D7 stays (radar zoom button).
RANGE=0x20-0x7E,0xB0,0xC0-0xF6,0xF8-0xFF
gen() { # name size extra-args...
  local name=$1 size=$2; shift 2
  npx --yes lv_font_conv --no-compress --no-kerning --bpp 4 --format lvgl --size "$size" \
    --lv-font-name "$name" "$@" -o "$OUT/$name.c"
  perl -0pi -e 's/#ifdef LV_LVGL_H_INCLUDE_SIMPLE.*?#endif/#include "lvgl.h"/s' "$OUT/$name.c"
}

gen font_14 14 --font "$TTF" --range $RANGE --font "$FA" --range 0xF00C,0xF078
for s in 18 24; do
  gen font_$s $s --font "$TTF" --range $RANGE
done
gen font_clock_48 48 --font "$TTF" --symbols "0123456789:-"
# tabular numerals: equal advance for 0-9 so clocks/values do not jitter (asserts equality)
python3 tools/tabular_digits.py "$OUT"/font_14.c "$OUT"/font_18.c "$OUT"/font_24.c "$OUT"/font_clock_48.c
