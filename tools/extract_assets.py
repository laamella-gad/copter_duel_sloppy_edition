#!/usr/bin/env python3
"""Extract the game's graphics from the snapshot for the C version.

Usage: tools/extract_assets.py [copterduel.vsf] [build]

Writes, from the unpacked game in the snapshot:
  sprites.bin  $4400-$4cc0  sprite blocks $10-$32 (copters, rocket, explosion, title)
  charset.bin  $7800-$8000  the character set
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from build import snapshot_ram  # noqa: E402

ASSETS = [
    ("sprites.bin", 0x4400, 0x4cc0),
    ("charset.bin", 0x7800, 0x8000),
]


def main():
    vsf = sys.argv[1] if len(sys.argv) > 1 else "copterduel.vsf"
    out = sys.argv[2] if len(sys.argv) > 2 else "build"
    ram = snapshot_ram(vsf)
    os.makedirs(out, exist_ok=True)
    for name, start, end in ASSETS:
        path = os.path.join(out, name)
        with open(path, "wb") as f:
            f.write(ram[start:end])
        print(f"Writing {path}: ${start:04x}-${end:04x}")


if __name__ == "__main__":
    main()
