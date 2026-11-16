#!/usr/bin/env bash
# Regenerates src/ui/fonts/*.c from tools/fonts/Montserrat-Medium.ttf (needs node). Outputs are committed.
set -euo pipefail
cd "$(dirname "$0")/.."
TTF=tools/fonts/Montserrat-Medium.ttf
OUT=src/ui/fonts
mkdir -p "$OUT"
gen() { # name size extra-args...
  local name=$1 size=$2; shift 2
  npx --yes lv_font_conv --no-compress --font "$TTF" --bpp 4 --format lvgl --size "$size" \
    --lv-font-name "$name" "$@" -o "$OUT/$name.c"
  # fallback to the built-in Montserrat 14
  sed -i "s/\.fallback = NULL/.fallback = \&lv_font_montserrat_14/" "$OUT/$name.c"
  perl -0pi -e 's/#ifdef LV_LVGL_H_INCLUDE_SIMPLE.*?#endif/#include "lvgl.h"/s' "$OUT/$name.c"
}

for s in 14 18 24; do
  gen font_$s $s --range 0x20-0x7F,0xA0-0xFF
done
gen font_clock_48 48 --symbols "0123456789:-"
# tabular numerals: equal advance for 0-9 so clocks/values do not jitter (asserts equality)
python3 tools/tabular_digits.py "$OUT"/font_14.c "$OUT"/font_18.c "$OUT"/font_24.c "$OUT"/font_clock_48.c
