// The explosion: one sprite that crashing copters and rockets hitting the
// background move and start over
#ifndef EXPLOSION_H
#define EXPLOSION_H

#define EXPLOSION_SPRITE    4

// Sprite blocks of the explosion's frames, the last one empty
#define EXPLOSION_FIRST     0x21
#define EXPLOSION_LAST      0x2a

// Passes per frame
#define EXPLOSION_FRAME     9

void explosion_reset(void);

// Moves the explosion to a sprite position and starts it over, with its sound
void explode(char x, char y);

// Each pass: the explosion's next frame every EXPLOSION_FRAME passes; after
// the last it is gone and its sound fades out
void explosion_update(void);

#pragma compile("explosion.c")

#endif
