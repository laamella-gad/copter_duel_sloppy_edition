# Copter Duel notes

"Copter Duel", (c) 1987 Ronald Mayer (Austria), published as "mr-soft presents": a two-player helicopter duel for the
C64. This project is the game rewritten in C for oscar64. The original was a BASIC program plus a machine code engine
(the round, at $8000) and window routines (at $8800); it was disassembled from a cracked tape image first. That work
(the T64, VICE snapshots, the Deflower annotations `copterduel.flo`, the BASIC listing, and the byte-patch build) is
in the git history up to commit 86dd7cd.

## Files

| File | What |
|---|---|
| `src/main.c` | Start: memory layout, graphics, VIC setup, then title and game in turn |
| `src/system.c` | Screen addresses, colours, joystick bits; text output, jiffy waits, key or button |
| `src/sound.c` | The SID: the copters' drone (voice 0), the explosion noise (voice 1), the rocket whoosh and the wall buzz (voice 2) |
| `src/title.c` | "mr-soft presents" and the title screen |
| `src/music.c` | The title music: the SID tune and its 50 Hz raster IRQ |
| `src/game.c` | One game: playfield and sprites set up, the round loop, the winner |
| `src/playfield.c` | Drawing the playfield; placing the bonuses |
| `src/panel.c` | The status panels: scores, energy and rocket bars, spare copters |
| `src/player.c` | The players' state; energy use and the points over time |
| `src/copter.c` | Flying, gravity, the rotor IRQ, crashes, copter collisions |
| `src/rocket.c` | Firing, flying, hitting the background or the other copter, picking up bonuses |
| `src/explosion.c` | The explosion sprite |
| `src/window.c` | Framed windows that save what they cover (the end of a game) |
| `src/sprites.bin` | Sprite blocks $10-$32, placed at $4400 |
| `src/charset.bin` | The character set, placed at $7800 |
| `src/gory_ghost.sid` | The title music, "Gory Ghost (preview)" (PSID, author unknown, 1993); its tune is placed at $5400 |
| `src/playfield_chars.bin` | The playfield's 1000 screen codes |
| `src/playfield_colours.bin` | The playfield's 1000 colours |
| `playfield.ctm` | The playfield as a CharPad project, for editing |
| `tools/test_end.py` | VICE test of the end-of-round message |
| `tools/ctm.py` | Converts the playfield to and from a CharPad project (`.ctm`) |

## Editing the playfield

Edit `playfield.ctm` in CharPad (Free 2.86 or Pro), save it, then
`tools/ctm.py import playfield.ctm src/playfield_chars.bin src/playfield_colours.bin [src/charset.bin]`.
`tools/ctm.py export src/playfield_chars.bin src/playfield_colours.bin src/charset.bin playfield.ctm` makes the
project again. Export writes CTM version 7, as CharPad Free saves; import reads versions 5, 7 and 9 (CharPad Pro).
Don't use CharPad's map export: it has no colours and cuts character numbers to a byte.

CharPad has no colour per screen cell, and CharPad Free no 1x1 tiles, so the project colours per character:
characters 0-255 are the charset, and each extra colour a character has on the playfield is a copy of it after 255.
On import a copy becomes the character below 256 with the same image and material; copies of characters that share
their image (the blank $3d and $20) get a material of their own. To use a character in a new colour, copy it to the
end of the set and colour the copy.

## Memory map

| Range | What |
|---|---|
| $ae/$af, $0334-$03ff | The title music's pointer and variables |
| $0801-$0900 | oscar64 startup |
| $0a00-$4000 | Code, data, variables, stack |
| $4000-$43ff | Screen (VIC bank 1: `$dd00` = 2, `$d018` = 15) |
| $43f8 | Sprite pointers: 0 yellow copter, 1 red copter, 2 yellow rocket, 3 red rocket, 4 explosion |
| $4049, $4251 | Player scores as 5 screen digits |
| $4400-$4cc0 | Sprites: $10-$1f copter (two rotor frames x 8 headings), $20 rocket, $21-$2a explosion, $2b-$32 title |
| $5400-$5b82 | Title music: init $5400 (A = 0), play $5403 |
| $7800-$7fff | Character set |

## The round (`game.c`)

- `play_round` points the IRQ at `rotor_irq` and clears the state; the loop runs until `game_over_flags`
  is set and both crashes are over. Bit 0 = yellow has no copters left (red wins), bit 1 = red.
- The round IRQ (`round_irq`): on the KERNAL's CIA IRQ it toggles the copters' rotor frame (bit 3 of the sprite
  pointer) every other IRQ, then chains to the KERNAL at $ea31. A raster IRQ at line 251 (below the playfield, new)
  runs the colour effects once a frame: `copters_flash` and `explosion_flash`.
- Yellow is the joystick in port 2 (`$dc00`), red the one in port 1 (`$dc01`). Each pass: `explosion_update`,
  `sound_update`, `copters_gravity`, `copter_control`, `rocket_move`, `player_tick`, `collisions`, `rocket_bonus`, `bonuses_update`
  (which acts every other pass). Within a step yellow goes first.
- Copter sprite blocks $10-$1f are 8 headings in two rotor frames (bit 3). With bit 3 set: $18 flying left, $19
  level left, $1a-$1d turning through the front (no firing), $1e level right, $1f flying right.
- Status bars are screen characters: energy and rockets go from $23 (full) to $27 (empty); copter icons are $5b, a lost one $3d.
- Sprite 4 is the explosion (multicolour, colour 8, frames $21-$2a, 9 passes each). A crashing copter or a rocket
  hitting the background moves it there and starts it over; there is one, so a new explosion cuts the running one
  short. It is only drawn: its collisions are ignored. Each explosion flashes the background (not the border)
  black, white, black for 2 frames each (new). Its sound is the noise on voice 1: each explosion turns the
  gate off and on to start it over, the end of the explosion turns it off.
- Firing a rocket starts a whoosh on voice 2 (new, the original was silent): noise falling from $3000 to $0800 in
  $0200 steps per pass, decay 300 ms.
- A copter touching the background buzzes (new): a 70 Hz square wave on voice 2, sustain 10, from the pass after
  the first touch until the pass after the last. The whoosh has voice 2 while it falls; the buzz comes back after it.
- A crash stops the copter and explodes it at its place (the copter stays visible, spinning through its 16 sprite
  blocks and flashing through the colours one per frame while the explosion runs, then hidden), and the other player gets 100 points; 19 explosion frames (171 passes) later a copter is lost. The original turned the copter sprite itself into the explosion.
- A rocket hitting the background explodes, unless its bonus cell holds a bonus; the player can fire again right
  away. The original's rocket just vanished.
- Every 250 passes a player gets 1 point and loses a unit of energy; out of energy is a crash.
- Each pass touching the background costs a unit of energy (a bar character holds 4, a full bar 28). The original
  crashed the copter after 2 passes of touching.
- A rocket hitting the other copter is gone and takes 9 units of energy (a third of a full bar); out of energy is
  a crash. In the original the hit crashed the copter and the rocket flew on.
- Rockets pick up bonuses: $3a 10 points, $3b 14 rockets (half a bar), $3c 10 energy (just over a rocket hit). The
  original gave 3 rockets and 5 energy.
- Copters stop sinking and moving down at y = $e8. A rocket picks up the bonus one row down and two columns right of
  its top left corner.

## Title wave

New: the original's title letters stood still. While the title waits, "copter" (sprites 0-5) waves up and down
around y = 80, 0-16 pixels from y = 72 out of a 256 byte sine table in `title.c`; each letter is 32 entries further on
and the wave moves 4 entries a frame (`vic_waitFrame`).

## Title music (`music.c`)

New: the original's title was silent. `title` starts the tune, which plays until a key or button is pressed; then
`music_stop` gives the IRQ back to the KERNAL and `sound_setup` resets the SID for the game. The tune is meant for 50
Hz, so a raster IRQ (line 250) calls play; the KERNAL's CIA IRQ (60 Hz) still runs for the keyboard scan, so the
IRQ routine checks `$d019` to tell them apart. Measured in VICE: 50 play calls in 60 jiffies.

## Timing

- The original round loop had no raster sync: one pass took 13136 cycles on average (measured in VICE, 12580-13616).
  The C loop is paced to that with CIA 2 timer A.
- BASIC FOR delays are KERNAL jiffies, measured in VICE: `forj=1to2000:next` is 124 jiffies, the title fade's
  `forj=1to70:nextj,i` about 7.

## Text output

The original printed everything through the KERNAL. The C version writes to the screen directly instead: texts are
screen code literals (`s"..."`) put at the row and column the original's cursor moves reached, the playfield is
`playfield_chars.bin` and `playfield_colours.bin` copied to the screen and colour RAM. Like the KERNAL's clear, `clear_screen` fills the colour RAM with
the colour of the last text. `@` (the copyright sign in this character set) is screen code 0, so it can't be in a
string and is written separately. Windows save what they cover into arrays (the original used the RAM under the
BASIC ROM).

## Fixed bugs

The original BASIC read the end flag from the wrong address (`peek(32941)` instead of 32950), so the winner was always
decided by points, even after a knockout. The C version uses `game_over_flags`: a knockout shows "red/yellow copter wins!".

After a game the title comes back without "mr-soft presents" first; the original showed it after every game. The
original's "please wait..." screen, shown while it loaded the engine, is left out.

Two quirks of the engine are fixed too:
- Yellow's down move and gravity stopped only at y = $e8 exactly (red's down move at y >= $e8); both copters now
  stop at y >= $e8.
- A rocket above row 0 or at the bottom row computed its bonus cell past the end of the screen row table, reading
  whatever bytes followed it; rockets off the screen grid now pick up nothing.

## Checked in VICE

Title and playfield (screen and colour RAM) matched the original, the end windows matched byte for byte, and
`tools/test_end.py` shows the right winner for flags 01 and 02.
