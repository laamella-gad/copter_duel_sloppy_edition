#!/usr/bin/env python3
"""Make copterduel.vsf: a VICE snapshot at the real game's entry point ($3181).

Usage: tools/snapshot.py copterduel.prg out.vsf

Runs x64sc in warp mode, drives its remote monitor through the packer layers
(see NOTES.md) and dumps a snapshot once the game is unpacked.
"""
import socket, subprocess, sys, time

prg, out = sys.argv[1], sys.argv[2]
vice = subprocess.Popen(
    ["x64sc", "-default", "-warp", "+sound",
     "-remotemonitor", "-remotemonitoraddress", "ip4://127.0.0.1:6510", prg],
    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
for _ in range(50):
    try:
        sock = socket.create_connection(("127.0.0.1", 6510))
        break
    except OSError:
        time.sleep(0.2)
sock.settimeout(1)


def cmd(command, wait=0.5):
    """Send a monitor command, wait, and print what VICE answers."""
    sock.sendall((command + "\n").encode())
    time.sleep(wait)
    buf = b""
    try:
        while True:
            data = sock.recv(65536)
            if not data:
                break
            buf += data
    except socket.timeout:
        pass
    print(buf.decode(errors="replace"))


# Stage 1 depacker (in $00f0-$01ef) ends with sei at $014a, jmp $0801.
cmd("break exec 014a")
cmd("x", 15)
cmd("delete")
# Stage 2 depacker runs from $0100 once stage 1 is done.
cmd("break exec 0100")
cmd("x", 5)
cmd("delete")
# Stage 2 jumps to the crack intro at $0810.
cmd("break exec 0810")
cmd("x", 10)
cmd("delete")
# Skip the intro's wait for space: go straight to its exit at $08e9, which
# moves $2000-$ffff down to $0801 and jumps to $3181.
cmd("r pc=08e9")
cmd("break exec 3181")
cmd("x", 10)
cmd("r")
cmd(f'dump "{out}"', 2)
cmd("quit", 1)
vice.wait(10)
