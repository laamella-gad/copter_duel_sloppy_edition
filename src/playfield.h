// The playfield: the screen with the status panels, and the bonuses on it
#ifndef PLAYFIELD_H
#define PLAYFIELD_H

#define BLANK           0x20

#define PLAYFIELD_BACK      LIGHT_BLUE

// Bonuses: 10 points, 14 rockets (half a bar), 10 energy (just over a rocket hit)
#define BONUS_POINTS    0x3a
#define BONUS_ROCKETS   0x3b
#define BONUS_ENERGY    0x3c

void playfield_draw(void);

bool is_bonus(char c);

void bonuses_reset(void);

// Each pass: every 402 passes the four bonuses move to new random places
void bonuses_update(void);

#pragma compile("playfield.c")

#endif
