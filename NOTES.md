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
     - $2ff0 → `$2c` (BIT abs) at $8499
     - $2ff1 → `$2c` at $8608
     - $2ff2 → `$3d` at $1700 and $1702 (in the BASIC program)

     The menu also always writes `$8f` to $09a7 (BASIC). The snapshot is from before the menu, so it holds the unpatched game.

## Memory map of the game

| Range | What |
|---|---|
| $0801-$175e | BASIC program: title, playfield drawn with PRINT, score screen, main loop |
| $2f00-$3229 | Crack: reset/RUN and trainer menu |
| $4000-$43ff | Screen (VIC bank 1: `poke56576,2`, `poke53272,15`, `poke648,64`) |
| $4000+ | Sprite pointers at $43f8 (`poke17400+i`) |
| $4049, $4251 | Player scores as 5 screen digits (BASIC reads `peek(16457+i)`, `peek(16977+i)`) |
| $7800-$7fff | Character set |
| $8000 | ML engine: `sys32768` plays one round |
| $80ad | End flag (`peek(32941)`): 1 = red wins, 2 = yellow wins |
| $8800 | `sysws,addr,w,h,c`: screen window routine (ws = 34816) |
| $8803 | `syswc` (wc = 34819) |

## BASIC structure (line numbers)

- 1000-1016: header, loads "copter" on first run (already in memory in this crack), VIC/SID setup
- 1017-1048: title screen ("mr-soft presents", sprites, "press any key or button to start")
- 1049-1078: game: `gosub1079` draws the screen, `sys32768` plays, then the winner and back to the title
- 1079-1109: playfield and status panels (energy, rockets, copters) for yellow (top) and red (bottom)

## Deflower status

0 differ. Still open: gaps in the ML at $803b-$8063, $8a2a-$8a6c and others, the ML's data and variables,
and code that jumps into the middle of instructions at $85d6/$85f2 (the BIT skip trick).
