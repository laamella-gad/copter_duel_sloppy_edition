# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

The C64 game "Copter Duel" (1987, Ronald Mayer) rewritten in C for [oscar64](~/dev/oscar64) (docs: `~/dev/oscar64/oscar64.md`), from a disassembly of the original. `NOTES.md` holds the details (memory map, engine, timing). Read it first and keep it up to date. The disassembly work is in the git history up to commit 86dd7cd.

## Commands

```sh
# Build: build/copterduel.prg (+ .asm, .map, .lbl listings)
make
# Build and run in VICE (X64=x64 etc. to pick another emulator)
make run
# Check the end-of-round message in VICE (01 = yellow knocked out, 02 = red)
tools/test_end.py build/copterduel.prg 01 /tmp/somewhere
```

## Architecture

`src/copterduel.c` is the original's BASIC program and includes `engine.c` (the machine code round) and `window.c` (screen windows) through `#pragma compile` in their headers. Keep it a faithful port: same screens, same timing; fixes of the original's bugs are listed in NOTES.md. Code and data live at $0a00-$4000 because VIC bank 1 ($4000-$7fff) holds the screen, the sprites (`src/sprites.bin` at $4400) and the character set (`src/charset.bin` at $7800), placed with `#pragma region` and `#embed`.

Test in VICE through its remote monitor (see `tools/test_end.py`, which finds addresses in the oscar64 listings).
