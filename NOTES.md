# Copter Duel notes

"Copter Duel", (c) 1987 Ronald Mayer (Austria), published as "mr-soft presents": a two-player helicopter duel
for the C64. This project is the game rewritten in C for oscar64. The original was a BASIC program plus a machine
code engine; it was disassembled from a cracked tape image first. That work (the T64, VICE snapshots, the Deflower
annotations `copterduel.flo`, the BASIC listing, and the byte-patch build) is in the git history up to commit
86dd7cd.

## Files

| File | What |
|---|---|
| `src/copterduel.c` | The original's BASIC program: title, playfield, end of round (comments give the BASIC line numbers) |
| `src/engine.c` | The original's machine code engine at $8000 (`sys32768`): one round |
| `src/window.c` | The original's window routines at $8800/$8803 (`sysws`/`syswc`) |
| `src/sprites.bin` | Sprite blocks $10-$32, placed at $4400 |
| `src/charset.bin` | The character set, placed at $7800 |
| `src/playfield.bin` | The playfield: 1000 screen codes, then 1000 colours |
| `tools/test_end.py` | VICE test of the end-of-round message |

## Memory map

| Range | What |
|---|---|
| $0801-$0900 | oscar64 startup |
| $0a00-$4000 | Code, data, variables, stack |
| $4000-$43ff | Screen (VIC bank 1: `$dd00` = 2, `$d018` = 15) |
| $43f8 | Sprite pointers: 0 yellow copter, 1 red copter, 2 yellow rocket, 3 red rocket |
| $4049, $4251 | Player scores as 5 screen digits |
| $4400-$4cc0 | Sprites: $10-$1f copter (two rotor frames x 8 headings), $20 rocket, $21-$2a explosion, $2b-$32 title |
| $7800-$7fff | Character set |

## The round (`engine.c`)

- `play_round` points the IRQ at `irq_rotor_animation` and clears the state; the loop runs until `game_over_flags`
  is set and both explosions are over. Bit 0 = yellow has no copters left (red wins), bit 1 = red.
- The rotor IRQ toggles the copters' rotor frame (bit 3 of the sprite pointer) every other IRQ, then chains to the KERNAL at $ea31.
- Yellow is the joystick in port 2 (`$dc00`), red the one in port 1 (`$dc01`). Each pass: `gravity`, `control`,
  `rocket_move`, `tick`, `collisions`, `rocket_bonus`, and every other pass `place_bonuses`.
- Status bars are screen characters: energy and rockets go from $23 (full) to $27 (empty); copter icons are $5b, a lost one $3d.
- A hit runs the explosion; afterwards a copter is lost, and the other player gets 100 points.
- Every 250 passes a player gets 1 point and loses a unit of energy; out of energy is a crash.
- Rockets pick up bonuses: $3a 10 points, $3b 3 rockets, $3c 5 energy.
- Copters stop sinking and moving down at y = $e8. A rocket picks up the bonus one row down and two columns right of
  its top left corner.

## Timing

- The original round loop had no raster sync: one pass took 13136 cycles on average (measured in VICE, 12580-13616).
  The C loop is paced to that with CIA 2 timer A.
- BASIC FOR delays are KERNAL jiffies, measured in VICE: `forj=1to2000:next` is 124 jiffies, the title fade's
  `forj=1to70:nextj,i` about 7.

## Text output

The original printed everything through the KERNAL. `copterduel.c` writes to the screen directly instead: texts are
screen code literals (`s"..."`) put at the row and column the original's cursor moves reached, the playfield is
`playfield.bin` copied to the screen and colour RAM. Like the KERNAL's clear, `clear_screen` fills the colour RAM with
the colour of the last text. `@` (the copyright sign in this character set) is screen code 0, so it can't be in a
string and is written separately. Windows save what they cover into arrays (the original used the RAM under the
BASIC ROM).

## Fixed bugs

The original BASIC read the end flag from the wrong address (`peek(32941)` instead of 32950), so the winner was always
decided by points, even after a knockout. The C version uses `game_over_flags`: a knockout shows "red/yellow copter wins!".

Two quirks of the engine are fixed too:
- Yellow's down move and gravity stopped only at y = $e8 exactly (red's down move at y >= $e8); both copters now
  stop at y >= $e8.
- A rocket above row 0 or at the bottom row computed its bonus cell past the end of the screen row table, reading
  whatever bytes followed it; rockets off the screen grid now pick up nothing.

## Checked in VICE

Title and playfield (screen and colour RAM) matched the original, the end windows matched byte for byte, and
`tools/test_end.py` shows the right winner for flags 01 and 02.
