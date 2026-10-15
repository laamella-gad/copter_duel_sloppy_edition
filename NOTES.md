# Copter Duel disassembly notes

Disassembled with [Deflower](../deflower): `../deflower/deflower --verify copterduel.flo` writes `out/copterduel.asm`.

## Files

| File | What |
|---|---|
| `COPTERDU.T64` | Original tape image: one PRG, "COPTER DUEL+", $0801-$2846, data at T64 offset $60 |
| `copterduel.prg` | That PRG, extracted from the T64 |
| `copterduel.vsf` | VICE snapshot at the game's entry point $3181 (everything unpacked) |
| `intro.vsf` | VICE snapshot at the crack intro's entry point $0810 |
| `copterduel.flo` | Deflower annotations for `copterduel.vsf` |
| `copterduel_basic.txt` | The game's BASIC program, listed with `petcat -2` |
| `tools/build.py` | Builds `build/copterduel.prg` from the snapshot, with the fixes: `LOAD`, `RUN` |
| `tools/test_end.py` | VICE test of the end-of-round message (original or C build) |
| `src/` | The game rewritten in C for oscar64: `make` → `build/copterduel_c.prg` |
| `tools/extract_assets.py` | Writes the sprites and character set from the snapshot to `build/` for the C build |
| `tools/snapshot.py` | Remakes `copterduel.vsf`: `tools/snapshot.py copterduel.prg copterduel.vsf` |

## The game

"Copter Duel", (c) 1987 Ronald Mayer (Austria), published as "mr-soft presents". This copy was cracked by TWG,
with an intro by Terrance & Phillip and a trainer (the "+" in the name).

## How the PRG unpacks

1. **BASIC stub** `SYS 2059` → $080b.
2. **Stage 1 depacker**: copies 256 bytes from $2777 to $00f0, jumps $014e; unpacks, then
   `lda #$37 / sta $01 / sei / jmp $0801` (the `sei` is at $014a).
3. **Stage 2 depacker** at $0801: moves data up to $a34b+, copies a depacker to $00fc, jumps $0100;
   unpacks, then `jmp $0810`.
4. **Crack intro** at $0810: screen from $111e (colours $1506), scroll text, music ($099c init, $09c6 play),
   waits for space. Exit at $08e9: copies $091a-$0a1a to $0334 and jumps there.
5. **Mover** at $0334: with $01 = $30 (all RAM) copies $2000-$ffff down to $0801, then `jmp $3181`.
6. **Crack start** $3181: clears SID, `jmp $2f00`.
   - $2f00: resets like the KERNAL does, calls the trainer menu, sets the BASIC pointers to $175e,
     then CLR ($a659), relink ($a533) and RUN ($a7ae).
   - $3100 trainer menu: three y/n questions (input routine $3200), answers in $2ff0-$2ff2. A "yes" patches:
     - "Unlimited lives for Texans?" (yellow): `$2c` (BIT abs) at $8499 `yellow_lose_copter`, so no icon is crossed out
     - "No points for Canadians?" (red): `$2c` at $8608 `red_score_inc`
     - "1 life only for Canadians?": `$3d` (lost copter) at $1700 and $1702, two of red's icons in BASIC line 1107

     The menu also always writes `$8f` (REM) to $09a7, over the `LOAD` of BASIC line 1011. The snapshot is from before the menu, so it holds the unpatched game.

## Memory map of the game

| Range | What |
|---|---|
| $0801-$175e | BASIC program: title, playfield drawn with PRINT, score screen, main loop |
| $2f00-$3229 | Crack: reset/RUN and trainer menu |
| $4000-$43ff | Screen (VIC bank 1: `poke56576,2`, `poke53272,15`, `poke648,64`) |
| $43f8 | Sprite pointers (`poke17400+i`): 0 yellow copter, 1 red copter, 2 yellow rocket, 3 red rocket |
| $4049, $4251 | Player scores as 5 screen digits (BASIC reads `peek(16457+i)`, `peek(16977+i)`) |
| $4400-$4cc0 | Sprites: $10-$1f copter (two rotor frames x 8 headings), $20 rocket, $21-$2a explosion, $2b-$32 title |
| $7800-$7fff | Character set |
| $8000 | ML engine: `sys32768` plays one round |
| $80b6 | `game_over_flags`: bit 0 = yellow has no copters left, bit 1 = red (see the bug below) |
| $8800 | `sysws,addr,w,h,c`: screen window routine (ws = 34816) |
| $8803 | `syswc` (wc = 34819) |

## The engine ($8000-$8a6c)

Everything is labelled in `copterduel.flo`. In short:
- `game_main` points the IRQ at `irq_rotor_animation` and clears the state; `round_loop` runs once per frame until
  `game_over_flags` is set and both explosions are over.
- Yellow is the joystick in port 2 (`$dc00`), red the one in port 1 (`$dc01`). Each frame: `gravity`, `*_control`,
  `*_rocket_move`, `*_tick`, `collisions`, `*_rocket_bonus`, and every other frame `place_bonuses`.
- Status bars are screen characters: energy and rockets go from $23 (full) to $27 (empty); copter icons are $5b, a lost one $3d.
- A hit (`yellow_hit`/`red_hit`) runs the explosion; afterwards a copter is lost, and the other player gets 100 points.
- Every 250 frames a player gets 1 point and loses a unit of energy (`*_tick`); out of energy is a crash.
- Rockets pick up bonuses: $3a 10 points, $3b 3 rockets, $3c 5 energy.
- `yellow_score_add_1/10/100` share code: `ldx #4`, then a `$2c` byte (BIT abs) that skips the next `ldx`. That's why
  deflower warns about jumps into instructions at $85d7/$85da and $85f3/$85f6. The warnings are expected.
- `window_open`/`window_close` ($8800/$8803) are the BASIC's `sysws`/`syswc`: framed windows with a stack of 6,
  saving the covered screen into the RAM under the BASIC ROM ($a000 characters, $b800 colours).

## Fixed bugs

Patches live in `PATCHES` in `tools/build.py`.

- BASIC line 1060 read the end flag with `peek(32941)` = $80ad, which is a branch operand inside `round_loop`
  (always 3), so the winner is always decided by points. The flag is `game_over_flags` at $80b6 (32950):
  1 = yellow lost all copters (red wins), 2 = red did. The BASIC's own test (`x=1` red wins, `x=2` yellow wins) matches it.
  Fixed at $0f89: `32941` → `32950`. Checked with `tools/test_end.py`: a knockout now shows "red/yellow copter wins!", where it showed "equal points!" before.

## BASIC structure (line numbers)

- 1000-1016: header, loads "copter" on first run (already in memory in this crack), VIC/SID setup
- 1017-1048: title screen ("mr-soft presents", sprites, "press any key or button to start")
- 1049-1078: game: `gosub1079` draws the screen, `sys32768` plays, then the winner and back to the title
- 1079-1109: playfield and status panels (energy, rockets, copters) for yellow (top) and red (bottom)

## The C version (`src/`)

The whole game without the crack intro and trainer, with the line 1060 fix, compiled with oscar64 (`make`).
- `copterduel.c` is the BASIC program (comments give the line numbers): it prints through the KERNAL (`$ffd2`)
  with the same PETSCII strings, so the screen editor draws the same screens. BASIC FOR delays are KERNAL jiffies:
  measured in VICE, `forj=1to2000:next` is 124 jiffies and the title fade's `forj=1to70:nextj,i` about 7.
- `engine.c` is the engine at $8000, one function per routine; the yellow/red copies of a routine are one function
  taking a `struct Player`. Differences kept on purpose: yellow's down move stops only at y = $e8, red's at y >= $e8;
  the row table is read past its end for rockets above row 0, so the bytes after it are kept.
- The rotor animation IRQ is the original routine as inline assembler, chained to the KERNAL at $ea31.
- `round_loop` had no raster sync: one pass took 13136 cycles on average (measured in VICE, 12580-13616).
  The C loop is paced to that with CIA 2 timer A.
- `window.c` is `sysws`/`syswc`, saving into arrays instead of the RAM under the BASIC ROM.
- Memory: code and variables at $0a00-$4000; sprites at $4400 and the character set at $7800 are `#embed`ded from
  `build/sprites.bin`/`build/charset.bin` (made from the snapshot by `tools/extract_assets.py`).
- Checked in VICE: title and playfield (screen and colour RAM) match the original, the end windows match byte for byte,
  and `tools/test_end.py build/copterduel_c.prg 01|02 ...` shows "red/yellow copter wins!".

## Deflower status

0 differ, 0 gaps; every routine, variable and table is labelled. The only warnings are from the BIT skip trick.
