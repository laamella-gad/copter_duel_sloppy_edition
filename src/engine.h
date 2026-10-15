// The machine code engine: sys32768 in the original (round_loop at $8000)
#ifndef ENGINE_H
#define ENGINE_H

#define screen          ((volatile char *)0x4000)
#define colour_ram      ((volatile char *)0xd800)
#define sprite_pointers ((volatile char *)0x43f8)

// The player scores: 5 screen digits each
#define yellow_score    ((volatile char *)0x4049)
#define red_score       ((volatile char *)0x4251)

// Bit 0: yellow has no copters left, bit 1: red
extern char game_over_flags;

// Plays one round; returns when a player has lost all copters
void play_round(void);

#pragma compile("engine.c")

#endif
