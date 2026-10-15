#!/usr/bin/env python3
"""Test the end-of-round message in VICE.

Usage: tools/test_end.py build/copterduel.prg <flag> <tag> <work dir>
       tools/test_end.py build/copterduel_c.prg <flag> <tag> <work dir>

Original: runs a copy of the PRG with the title's key wait (line 1048) made
a REM, skips the trainer questions, sets game_over_flags ($80b6) to <flag>
(01 = yellow knocked out, 02 = red) at the first round_loop, and prints the
screen lines with the result. Tag `unfixed` undoes the line 1060 fix.

C version (an oscar64 build, with its .asm and .lbl next to the PRG): skips
the title's key wait and sets game_over_flags in the first pass of the
round loop, found in the listing.
"""
import os, re, socket, subprocess, sys, time

src, flag, tag = sys.argv[1], sys.argv[2], sys.argv[3]
S = sys.argv[4]
base = os.path.splitext(src)[0]
c_version = os.path.exists(base + '.lbl')
if c_version:
    prg = src
    asm = open(base + '.asm').read()
    labels = {l.split()[2][1:]: l.split()[1] for l in open(base + '.lbl')}
    # the title's wait: lda $cb / cmp #$40 / bne <out>
    key_wait = re.search(r'([0-9a-f]{4}) : a5 cb .*\n.*\n.*BNE \$([0-9a-f]{4})', asm)
    # the round loop's pace: lda $dd0d / lsr / bcc, then the pass
    loop = re.search(r'ad 0d dd .*\n.*LSR\n.*BCC.*\n.*\n([0-9a-f]{4}) :', asm)
else:
    d = bytearray(open(src,'rb').read())
    off = lambda a: a - 0x0801 + 2
    assert d[off(0x0daa)] == 0x8b; d[off(0x0daa)] = 0x8f      # line 1048: IF -> REM
    if tag == 'unfixed':
        d[off(0x0f89):off(0x0f89)+5] = b'32941'
    prg = f'{S}/test_{tag}.prg'; open(prg,'wb').write(d)
p = subprocess.Popen(["x64sc","-default","-warp","+sound","-remotemonitor","-remotemonitoraddress","ip4://127.0.0.1:6510",
                      "-autostartprgmode","1", prg],
                     stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
for _ in range(50):
    try: s = socket.create_connection(("127.0.0.1",6510)); break
    except OSError: time.sleep(0.2)
s.settimeout(1)
def cmd(c, wait=0.5):
    s.sendall((c+"\n").encode()); time.sleep(wait); buf=b""
    try:
        while True:
            x=s.recv(65536)
            if not x: break
            buf+=x
    except socket.timeout: pass
    return buf.decode(errors="replace")
if c_version:
    cmd("break exec " + key_wait.group(1)); print(cmd("x", 20).strip().splitlines()[0:1])
    cmd("delete"); cmd("r pc=" + key_wait.group(2))  # skip the key wait
    cmd("break exec " + loop.group(1)); print(cmd("x", 20).strip().splitlines()[0:1])
    cmd("delete"); cmd(f"> {labels['game_over_flags']} {flag}")
else:
    cmd("break exec 3100"); print(cmd("x", 20).strip().splitlines()[0:1])
    cmd("delete"); cmd("r pc=317b")                     # skip trainer questions
    cmd("break exec 8063"); print(cmd("x", 20).strip().splitlines()[0:1])
    cmd("delete"); cmd(f"> 80b6 {flag}")                # game over flag
cmd("x", 8)
cmd(f'bsave "{S}/screen_{tag}.bin" 0 4000 43e7', 1)
cmd("quit", 1); p.wait(10)
scr = open(f'{S}/screen_{tag}.bin','rb').read()
def ch(c):
    c &= 0x7f
    if c == 0: return '@'
    if 1 <= c <= 26: return chr(c+96)
    if 32 <= c < 64: return chr(c)
    return '.'
for row in range(25):
    line = ''.join(ch(c) for c in scr[row*40:row*40+40])
    if any(w in line for w in ('wins', 'equal', 'points')): print(row, line)
