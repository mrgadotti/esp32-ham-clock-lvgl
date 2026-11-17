#!/usr/bin/env python3
"""Host side of the SCREEN_TEST firmware (env cyd_test): screenshots over serial + synthetic touch.

  tools/screen_test.py script.txt            run a script file
  tools/screen_test.py -c "goto 2" "shot dx" run inline lines
  tools/screen_test.py --all-screens         EN + PT shot of every screen (restores the original language)

Script lines (one per line, '#' comments):
  wait SECONDS | shot NAME | mem | contact NAME | soak SECONDS PERIOD | any firmware command (goto, tap, swipe, lang, autosw)
Exit code 1 if the serial log shows panic/abort/Guru/rst:/SSL-alloc lines, a reboot, or a shot could not be assembled.
"""
import argparse, re, struct, sys, time
from pathlib import Path

import serial
from PIL import Image, ImageDraw

W, H = 320, 240
MAGIC = bytes([0xA5, 0x5A, 0xC3, 0x3C])
FAIL = re.compile(r"Guru Meditation|abort\(\)|Backtrace:|panic|rst:|assert failed|Brownout|watchdog|"
                  r"SSL.*(alloc|memory)|alloc.*fail|Memory allocation failed|ssl_setup|E \(\d+\)", re.I)
SCREENS = [("hamclock", 1), ("dx", 2), ("sunspots", 3), ("bands", 4), ("weather", 5), ("aprslist", 6), ("pota", 7),
           ("hamalert", 8), ("aprsradar", 9), ("forecast", 12), ("location", 13), ("settings", 14)]


class Board:
    def __init__(self, port, baud, out, logpath):
        self.s = serial.Serial()
        self.s.port, self.s.baudrate, self.s.timeout = port, baud, 0.05
        self.s.dtr = self.s.rts = False  # opening must not reset the board
        self.s.open()
        self.buf = bytearray()
        self.out = Path(out)
        self.out.mkdir(parents=True, exist_ok=True)
        self.log = open(logpath, "a")
        self.fails, self.lines, self.rects, self.shots, self.ready = [], [], [], [], 0
        self.t0 = time.time()
        self.imgs = []

    def _text(self, raw):
        for ln in raw.decode("latin-1").replace("\r", "").split("\n"):
            if not ln.strip():
                continue
            self.log.write(f"{time.time() - self.t0:8.1f} {ln}\n")
            self.lines.append(ln)
            if "[ST] ready" in ln:
                self.ready += 1
                if self.ready > 1:
                    self.fails.append("reboot detected: " + ln)
            elif FAIL.search(ln) and not ln.startswith("[ST]"):
                self.fails.append(ln)
                print("FAIL LINE:", ln, file=sys.stderr)

    def pump(self, secs=0.0):
        end = time.time() + secs
        while True:
            d = self.s.read(65536)
            self.buf += d
            while True:
                i = self.buf.find(MAGIC)
                if i < 0:
                    nl = self.buf.rfind(b"\n")  # a partial magic can only sit after the last newline
                    if nl >= 0:
                        self._text(bytes(self.buf[:nl + 1]))
                        del self.buf[:nl + 1]
                    break
                if i > 0:
                    self._text(bytes(self.buf[:i]))
                    del self.buf[:i]
                if len(self.buf) < 12:
                    break
                x1, y1, x2, y2 = struct.unpack("<4H", self.buf[4:12])
                if not (0 <= x1 <= x2 < W and 0 <= y1 <= y2 < H):
                    del self.buf[:4]  # not a real frame header
                    continue
                n = (x2 - x1 + 1) * (y2 - y1 + 1) * 2
                if len(self.buf) < 12 + n + 2:
                    break
                px = bytes(self.buf[12:12 + n])
                (sm,) = struct.unpack("<H", self.buf[12 + n:14 + n])
                del self.buf[:14 + n]
                self.rects.append((x1, y1, x2, y2, px, sum(px) & 0xFFFF == sm))
            if time.time() >= end:
                return

    def send(self, line, ack=True, timeout=5):
        self.pump(0)
        self.s.write((line + "\n").encode())
        if not ack:
            return
        cmd = line.split()[0]
        mark = len(self.lines)
        end = time.time() + timeout
        while time.time() < end:
            self.pump(0.1)
            for ln in self.lines[mark:]:
                if ln.startswith("[ST] ok " + cmd) or ln.startswith("[ST] err"):
                    return ln
        self.fails.append(f"no ack for '{line}'")

    def mem(self):
        mark = len(self.lines)
        self.send("mem", ack=False)
        end = time.time() + 5
        while time.time() < end:
            self.pump(0.1)
            for ln in self.lines[mark:]:
                m = re.search(r"\[ST\] mem lvfree=(\d+) free=(\d+) largest=(\d+)", ln)
                if m:
                    return ln, int(m.group(1)), int(m.group(3))
        self.fails.append("no mem reply")
        return "", 1 << 30, 1 << 30

    def shot(self, name, tries=3):
        for _ in range(tries):
            self.rects.clear()
            mark = len(self.lines)
            self.send("shot", ack=False)
            end = time.time() + 15
            while time.time() < end and not any(l.startswith("[ST] shot_end") for l in self.lines[mark:]):
                self.pump(0.1)
            self.pump(0.2)
            img = Image.new("RGB", (W, H))
            cover = [[False] * W for _ in range(H)]
            ok = bool(self.rects) and all(r[5] for r in self.rects)
            for x1, y1, x2, y2, px, _ in self.rects:
                w, h = x2 - x1 + 1, y2 - y1 + 1
                for j in range(w * h):
                    v = px[2 * j] | (px[2 * j + 1] << 8)
                    r, g, b = (v >> 11) & 31, (v >> 5) & 63, v & 31
                    x, y = x1 + j % w, y1 + j // w
                    img.putpixel((x, y), ((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2)))
                    cover[y][x] = True
            if ok and all(all(row) for row in cover):
                p = self.out / f"{name}.png"
                img.save(p)
                self.imgs.append((name, img))
                print("saved", p)
                return p
        self.fails.append(f"shot '{name}' failed (corrupt or incomplete frame)")
        return None

    def contact(self, name, cols=4):
        if not self.imgs:
            return
        rows = (len(self.imgs) + cols - 1) // cols
        sheet = Image.new("RGB", (cols * (W + 4), rows * (H + 16)), (40, 40, 40))
        d = ImageDraw.Draw(sheet)
        for i, (n, im) in enumerate(self.imgs):
            x, y = (i % cols) * (W + 4), (i // cols) * (H + 16)
            d.text((x + 2, y + 2), n, fill=(255, 255, 0))
            sheet.paste(im, (x, y + 14))
        sheet.save(self.out / f"{name}.png")

    def soak(self, secs, period):
        order = [s[1] for s in SCREENS]
        t_end, i, minlv = time.time() + secs, 0, 1 << 30
        self.send("autosw 0")
        while time.time() < t_end:
            sid = order[i % len(order)]
            i += 1
            self.send(f"goto {sid}")
            self.pump(period - 3.2)
            ln, lv, largest = self.mem()
            print(f"soak screen {sid}: {ln}")
            minlv = min(minlv, lv)
            if lv < 8192:
                self.fails.append(f"lvfree {lv} < 8 KB on screen {sid}")
            self.pump(3)
        print("soak min lvfree", minlv)

    def run_line(self, line):
        line = line.split("#")[0].strip()
        if not line:
            return
        a = line.split()
        if a[0] == "wait":
            self.pump(float(a[1]))
        elif a[0] == "shot":
            self.shot(a[1])
        elif a[0] == "mem":
            print(self.mem()[0])
        elif a[0] == "contact":
            self.contact(a[1])
        elif a[0] == "soak":
            self.soak(float(a[1]), float(a[2]))
        else:
            r = self.send(line)
            print(">", line, "->", r)


def all_screens():
    out, orig = [], None
    out.append("autosw 0")
    for lang in ("en", "pt"):
        out.append(f"lang {lang}")
        out.append("goto 14")  # make sure the first screen below is rebuilt in the new language
        out.append("wait 1.5")
        for name, sid in SCREENS:
            out += [f"goto {sid}", "wait 2.5", f"shot {name}_{lang}"]
    out.append("contact contact_sheet")
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("script", nargs="?")
    ap.add_argument("-c", nargs="+")
    ap.add_argument("--all-screens", action="store_true")
    ap.add_argument("--sheet", action="store_true", help="rebuild contact_sheet.png from the saved <screen>_<lang>.png files, no board")
    ap.add_argument("--port", default="/dev/ttyUSB0")
    ap.add_argument("--baud", type=int, default=921600)
    ap.add_argument("--out", default="docs/review/screens")
    ap.add_argument("--log", default="docs/review/serial.log")
    ap.add_argument("--restore-lang", type=int, default=None, help="firmware lang to restore at the end (0 en, 1 pt)")
    a = ap.parse_args()
    if a.sheet:
        b = Board.__new__(Board)
        b.out, b.imgs = Path(a.out), []
        for lang in ("en", "pt"):
            for name, _ in SCREENS:
                b.imgs.append((f"{name}_{lang}", Image.open(b.out / f"{name}_{lang}.png").convert("RGB")))
        b.contact("contact_sheet")
        return
    lines = all_screens() if a.all_screens else (a.c or (Path(a.script).read_text().splitlines() if a.script else []))
    b = Board(a.port, a.baud, a.out, a.log)
    b.pump(2.0)
    if b.ready:  # opening the port resets the board: let the boot screen finish
        b.pump(14.0)
    b.send("ping")
    orig = None
    for ln in lines:
        b.run_line(ln)
        for l in b.lines[-6:]:
            m = re.search(r"\[ST\] lang was=(\d)", l)
            if m and orig is None:
                orig = int(m.group(1))
    if orig is not None or a.restore_lang is not None:
        b.send(f"lang {'pt' if (a.restore_lang if a.restore_lang is not None else orig) else 'en'}")
    b.pump(1)
    print(f"{len(b.fails)} failure(s)")
    for f in b.fails:
        print(" FAIL:", f)
    sys.exit(1 if b.fails else 0)


if __name__ == "__main__":
    main()
