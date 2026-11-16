#!/usr/bin/env python3
"""Make '0'-'9' tabular in lv_font_conv .c output: widest digit adv_w for all, bitmap centered. Idempotent."""
import re, sys


def process(path):
    s = open(path).read()
    cm = re.search(r"cmaps\[\]\s*=\s*\{(.*?)\n\};", s, re.S).group(1)
    gids = {}
    for m in re.finditer(r"\{\s*\.range_start = (\d+), \.range_length = (\d+), \.glyph_id_start = (\d+),\s*\.unicode_list = NULL, \.glyph_id_ofs_list = (\w+),.*?\.type = (\w+)", cm, re.S):
        start, length, g0, ofs = int(m[1]), int(m[2]), int(m[3]), m[4]
        lst = None
        if ofs != "NULL":
            raw = re.search(ofs + r"\[\] = \{(.*?)\};", s, re.S).group(1)
            lst = [int(x) for x in raw.split(",") if x.strip()]
        for cp in range(ord("0"), ord("9") + 1):
            if start <= cp < start + length:
                gids[cp] = g0 + (lst[cp - start] if lst else cp - start)
    assert len(gids) == 10, f"{path}: digits not found"
    body = re.search(r"glyph_dsc\[\] = \{\n(.*?)\n\};", s, re.S)
    lines = body.group(1).split("\n")

    def adv(i):
        return int(re.search(r"\.adv_w = (\d+)", lines[i])[1])

    wmax = max(adv(g) for g in gids.values())
    for g in gids.values():
        bw = int(re.search(r"\.box_w = (\d+)", lines[g])[1])
        lines[g] = re.sub(r"\.adv_w = \d+", f".adv_w = {wmax}", lines[g])
        lines[g] = re.sub(r"\.ofs_x = -?\d+", f".ofs_x = {round((wmax / 16 - bw) / 2)}", lines[g])
    assert len({adv(g) for g in gids.values()}) == 1
    open(path, "w").write(s[:body.start(1)] + "\n".join(lines) + s[body.end(1):])
    print(f"{path}: digit adv_w = {wmax}")


for p in sys.argv[1:]:
    process(p)
