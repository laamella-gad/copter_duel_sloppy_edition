#!/usr/bin/env python3
"""Build a runnable, patched Copter Duel from the snapshot.

Usage: tools/build.py [copterduel.vsf] [build/copterduel.prg]

Takes the unpacked game from the snapshot ($0801-$8a6c: BASIC program,
crack, sprites, charset, engine), applies PATCHES, and writes a PRG that
loads at $0801 and starts with RUN.

The PRG starts with a `SYS 12673` stub over the first bytes of the BASIC
program. That jumps to crack_start ($3181), which resets the machine, shows
the trainer menu, restores the BASIC program's first 64 bytes from
crack_basic_head ($2ec0), sets the BASIC pointers and RUNs it, just as the
original did after its intro.
"""
import sys

START, END = 0x0801, 0x8a6c
CRACK_START = 0x3181

# (address, original bytes, new bytes, why)
PATCHES = [
    (0x0f89, b"32941", b"32950",
     "BASIC line 1060: read the end flag from game_over_flags ($80b6), "
     "not from $80ad inside round_loop, so a knockout decides the winner"),
]


def snapshot_ram(path):
    """The 64K of RAM in a VICE snapshot's C64MEM module."""
    data = open(path, "rb").read()
    i = data.find(b"C64MEM")
    if i < 0:
        sys.exit(f"{path}: no C64MEM module")
    # module name (16), version (2), size (4), cpu port data/dir, exrom, game (4)
    ram = data[i + 26:i + 26 + 0x10000]
    return bytearray(ram)


def sys_stub(address):
    """A BASIC line `0 SYS<address>` at $0801, then the end of the program."""
    line = b"\x00\x00\x9e" + str(address).encode() + b"\x00"
    link = START + 2 + len(line)
    return bytes([link & 0xff, link >> 8]) + line + b"\x00\x00"


def main():
    vsf = sys.argv[1] if len(sys.argv) > 1 else "copterduel.vsf"
    out = sys.argv[2] if len(sys.argv) > 2 else "build/copterduel.prg"
    ram = snapshot_ram(vsf)
    for address, old, new, why in PATCHES:
        found = bytes(ram[address:address + len(old)])
        if found != old:
            sys.exit(f"${address:04x}: expected {old!r}, found {found!r}")
        ram[address:address + len(new)] = new
        print(f"${address:04x}: {why}")
    image = bytearray(ram[START:END])
    stub = sys_stub(CRACK_START)
    image[:len(stub)] = stub
    with open(out, "wb") as f:
        f.write(bytes([START & 0xff, START >> 8]) + image)
    print(f"Writing {out}: ${START:04x}-${END:04x}")


if __name__ == "__main__":
    main()
