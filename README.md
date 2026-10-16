# Copter Duel Sloppy Edition

A two-player helicopter duel for the Commodore 64: yellow (joystick in port 2) and red (port 1) fight it out with
rockets in a cave.

## The original

"Copter Duel" was written in 1987 by Ronald Mayer and published as "mr-soft": a BASIC program with a
machine code engine. Copter Duel Sloppy Edition is a rewrite in C for [oscar64](https://github.com/drmortalwombat/oscar64), made from a
disassembly of a tape image of the original. The tape image, the disassembly and the patched original are in the git
history up to commit 86dd7cd. `NOTES.md` has the details.

## Improvements

- The winner is decided right: the original read the end flag from the wrong address and always picked the player
  with the most points, even after a knockout.
- The copters have momentum instead of moving at a fixed speed.
- Touching the cave walls drains energy instead of crashing the copter at once.
- A rocket hit takes a third of the energy instead of crashing the copter, and the bonuses give more rockets and energy.
- Rockets explode when they hit the cave walls.
- Crashes are more dramatic: the copter spins and flashes while it explodes, the background flashes on every explosion.
- New sounds: a whoosh when a rocket is fired and a buzz while a copter touches a wall.
- The title screen plays music ("Gory Ghost") and its letters wave.
- Fixed engine quirks: copters stopped at the ground inconsistently, and rockets near the screen edges read memory
  past the screen.
- The whole program is now written in C (Oscar64 compatible)

## Building

Needs oscar64 and pucrunch; `make run` also needs VICE.

```sh
make        # build/copterduel_crunched.prg
make run    # build and start it in VICE
```
