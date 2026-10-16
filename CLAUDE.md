# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Copter Duel Sloppy Edition: the C64 game "Copter Duel" (1987, Ronald Mayer) rewritten in C for [oscar64](~/dev/oscar64) (docs: `~/dev/oscar64/oscar64.md`), from a disassembly of the original. `NOTES.md` holds the details (memory map, engine, timing). Read it first and keep it up to date. The disassembly work is in the git history up to commit 86dd7cd.

## Commands

```sh
# Build: the pucrunched copterduel.prg (checked in), from build/copterduel_raw.prg
# (+ .asm, .map, .lbl listings)
make
# Build and run it in VICE (X64=x64 etc. to pick another emulator)
make run
```

## Architecture

`src/main.c` pulls in the other sources through `#pragma compile` in their headers; the files follow the game's parts (title, music, game, playfield, panel, player, copter, rocket, explosion, sound, window), listed in NOTES.md. Screens and timing follow the original; NOTES.md lists every difference (fixed bugs and gameplay changes). Code and data live at $0a00-$4000 because VIC bank 1 ($4000-$7fff) holds the screen, the sprites (`src/sprites.bin` at $4400), the title music (`src/gory_ghost.sid` at $5400, see `music.c`) and the character set (`src/charset.bin` at $7800), placed with `#pragma region` and `#embed`.
