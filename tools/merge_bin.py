#!/usr/bin/env python3
"""Merge bootloader, partitions, boot_app0 and app into one flashable image.

Usage: python tools/merge_bin.py <env>   (after `pio run -e <env>`)
Offsets match the huge_app.csv layout.
"""
import shutil
import subprocess
import sys
from pathlib import Path

PIO = Path.home() / ".platformio" / "packages"
BOOT_APP0 = PIO / "framework-arduinoespressif32" / "tools" / "partitions" / "boot_app0.bin"


def esptool_cmd():
    if shutil.which("esptool.py"):
        return ["esptool.py"]
    if subprocess.run([sys.executable, "-m", "esptool", "version"],
                      capture_output=True).returncode == 0:
        return [sys.executable, "-m", "esptool"]
    script = PIO / "tool-esptoolpy" / "esptool.py"
    if script.exists():
        return [sys.executable, str(script)]
    sys.exit("error: esptool not found (PATH, python -m esptool, or PlatformIO package)")


def main():
    if len(sys.argv) != 2:
        sys.exit(f"usage: {sys.argv[0]} <env>")
    env = sys.argv[1]
    build = Path(".pio") / "build" / env
    parts = [
        ("0x1000", build / "bootloader.bin"),
        ("0x8000", build / "partitions.bin"),
        ("0xe000", BOOT_APP0),
        ("0x10000", build / "firmware.bin"),
    ]
    for _, p in parts:
        if not p.is_file():
            sys.exit(f"error: missing {p} (run `pio run -e {env}` first)")
    out = Path("dist") / env / "firmware-merged.bin"
    out.parent.mkdir(parents=True, exist_ok=True)
    cmd = esptool_cmd() + ["--chip", "esp32", "merge_bin", "-o", str(out),
                           "--flash_mode", "dio", "--flash_size", "4MB"]
    for off, p in parts:
        cmd += [off, str(p)]
    subprocess.run(cmd, check=True)
    print(f"wrote {out}")


main()
