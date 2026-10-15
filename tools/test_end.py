#!/usr/bin/env python3
"""Test the end-of-round message in VICE.

Usage: tools/test_end.py build/copterduel.prg <flag> <work dir>

Runs the PRG in x64sc, skips the title's key wait, sets game_over_flags to
<flag> (01 = yellow knocked out, 02 = red) in the first pass of the round
loop, and prints the screen lines with the result. The addresses come from
the .asm and .lbl listings oscar64 writes next to the PRG.
"""
import os, re, socket, subprocess, sys, time

prg, flag, work = sys.argv[1], sys.argv[2], sys.argv[3]
base = os.path.splitext(prg)[0]
asm = open(base + '.asm').read()
labels = {l.split()[2][1:]: l.split()[1] for l in open(base + '.lbl')}
# the title's wait: lda $cb / cmp #$40 / bne <out>
key_wait = re.search(r'([0-9a-f]{4}) : a5 cb .*\n.*\n.*BNE \$([0-9a-f]{4})', asm)
# the round loop's pace: lda $dd0d / lsr / bcc, then the pass
loop = re.search(r'ad 0d dd .*\n.*LSR\n.*BCC.*\n.*\n([0-9a-f]{4}) :', asm)

p = subprocess.Popen(["x64sc", "-default", "-warp", "+sound",
                      "-remotemonitor", "-remotemonitoraddress", "ip4://127.0.0.1:6510",
                      "-autostartprgmode", "1", prg],
                     stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
for _ in range(50):
    try:
        s = socket.create_connection(("127.0.0.1", 6510))
        break
    except OSError:
        time.sleep(0.2)
s.settimeout(1)


def cmd(c, wait=0.5):
    s.sendall((c + "\n").encode())
    time.sleep(wait)
    buf = b""
    try:
        while True:
            x = s.recv(65536)
            if not x:
                break
            buf += x
    except socket.timeout:
        pass
    return buf.decode(errors="replace")


cmd("break exec " + key_wait.group(1)); print(cmd("x", 20).strip().splitlines()[0:1])
cmd("delete"); cmd("r pc=" + key_wait.group(2))     # skip the key wait
cmd("break exec " + loop.group(1)); print(cmd("x", 20).strip().splitlines()[0:1])
cmd("delete"); cmd(f"> {labels['game_over_flags']} {flag}")
cmd("x", 8)
screen = os.path.join(work, f"screen_{flag}.bin")
cmd(f'bsave "{screen}" 0 4000 43e7', 1)
cmd("quit", 1)
p.wait(10)

scr = open(screen, 'rb').read()


def ch(c):
    c &= 0x7f
    if c == 0: return '@'
    if 1 <= c <= 26: return chr(c + 96)
    if 32 <= c < 64: return chr(c)
    return '.'


for row in range(25):
    line = ''.join(ch(c) for c in scr[row * 40:row * 40 + 40])
    if any(w in line for w in ('wins', 'equal', 'points')):
        print(row, line)
