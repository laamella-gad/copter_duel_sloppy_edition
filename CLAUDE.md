# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A disassembly of the C64 game "Copter Duel" (1987, Ronald Mayer), taken from a cracked tape image (`COPTERDU.T64`), with the goal of understanding it and improving some features. The disassembly is `copterduel.flo`, annotations for the [Deflower](../deflower) disassembler. `src/` is the game rewritten in C for [oscar64](~/dev/oscar64) (docs: `~/dev/oscar64/oscar64.md`). `NOTES.md` holds everything learned so far (packer layers, crack/trainer, memory map, BASIC structure). Read it first and keep it up to date as you learn more.

## Commands

```sh
# Disassemble: writes out/copterduel.asm and out/copterduel.map (out/ is cleared each run)
../deflower/deflower --verify copterduel.flo
# Rerun on every save of the .flo
../deflower/deflower --watch --verify copterduel.flo

# Look at bytes / search / find text in the snapshot
../deflower/deflower --dump copterduel.vsf '$8000-$8100'
../deflower/deflower --find copterduel.vsf '$8800'
../deflower/deflower --strings copterduel.vsf

# Reassemble the output (-i allows illegal opcodes)
64tass -i -o out/copterduel.prg out/copterduel.asm

# Build the fixed game: build/copterduel.prg (LOAD, RUN; starts with the crack's trainer menu)
tools/build.py
# Check the end-of-round message in VICE (01 = yellow knocked out, 02 = red)
tools/test_end.py build/copterduel.prg 01 fixed /tmp/somewhere

# Build the C version: build/copterduel_c.prg (+ .asm, .map, .lbl listings)
make
# Same end-of-round test for the C version
tools/test_end.py build/copterduel_c.prg 01 c /tmp/somewhere

# Rebuild the snapshot from the PRG (runs x64sc in warp via its remote monitor)
tools/snapshot.py copterduel.prg copterduel.vsf

# List the BASIC part (the snapshot's RAM $0801-$175e, saved as a PRG) with VICE's petcat
petcat -2 -o copterduel_basic.txt -- basic.prg
```

A good run has `0 differ`. A run with errors (e.g. an area conflict) writes nothing. The `.flo` format, the output, and the warnings are described in `../deflower/README.md`.

## Architecture of the game

- `copterduel.vsf` is a VICE snapshot taken at $3181, after the two depacker stages, the crack intro, and the move of $2000-$ffff down to $0801. Disassemble from it, not from the PRG: the PRG is packed.
- The game itself is split in two:
  - a **BASIC program** at $0801-$175e (title, playfield drawn with PRINT, scores, round loop). It is not disassembled; see `copterduel_basic.txt`.
  - a **machine code engine**: `sys32768` ($8000) plays one round; $8800 and $8803 are the screen-window routines the BASIC calls as `sysws`/`syswc`.

  The two parts talk through fixed addresses (scores on screen at $4049/$4251, end flag `game_over_flags` at $80b6).
- The cracker's code at $2f00-$3229 (reset, BASIC RUN, trainer menu) is not part of the original game. Its trainer pokes are at $8499 (yellow's lost copter), $8608 (red's score), $1700/$1702 (red's copter icons) and $09a7 (the BASIC's LOAD becomes a REM).
- Video uses VIC bank 1: screen at $4000, character set at $7800.

## Making changes

The game isn't rebuilt from `out/copterduel.asm`. Fixes are byte patches in `PATCHES` in `tools/build.py`, applied to the snapshot's RAM, each one checking the original bytes. The PRG puts a `SYS 12673` stub over the BASIC head. crack_start ($3181) restores the head from $2ec0 and RUNs the BASIC program. Test fixes in VICE through the remote monitor (see `tools/test_end.py`).

## The C version

`src/copterduel.c` is the BASIC program and includes `engine.c` (the $8000 engine) and `window.c` (`sysws`/`syswc`) through `#pragma compile` in their headers. It must behave like the original: keep it a faithful port of the BASIC lines and routines in `copterduel.flo`, and check changes against the original in VICE (`tools/test_end.py` works on both builds). Code and data live at $0a00-$4000 because VIC bank 1 ($4000-$7fff) holds the screen, sprites and character set. See NOTES.md for timing and layout details.

## Deflower gotchas seen here

- `include c64.flo` pulls in `c64_system.flo`, which claims $0100-$0800 and stops tracing there. To disassemble code that runs in the tape buffer or stack (the depackers, the $0334 mover), extract it to a `.bin` and include only `c64_io.flo`, `c64_kernal.flo`, and `c64_zeropage.flo`.
- `--init` marks zeroed RAM as `unused_*` virtual ranges. The game's ML workspace ($3229-$4407) was zero in the snapshot, so delete such ranges when deflower warns that one is used.
