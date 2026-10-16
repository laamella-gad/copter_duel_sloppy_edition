// The copters: flying, gravity, the rotor, crashes
#ifndef COPTER_H
#define COPTER_H

#include "player.h"

// The start of a round: the rotor animation runs in the KERNAL's IRQ, the
// colour effects of explosions in a raster IRQ
void copters_start(void);

// The end of a round: the raster IRQ stops, the rotor animation goes on
void copters_stop(void);

// Each pass: every 31 passes both copters sink a pixel
void copters_gravity(void);

// Each pass: the joystick flies, turns and fires; a crashed copter goes on crashing
void copter_control(struct Player * p);

// Crashes the copter: it stops and explodes, the other player gets 100
// points. Later calls step the crash: afterwards the player loses a copter
// and starts over at home, or, without copters left, is out.
void copter_crash(struct Player * p);

// Touching the background costs a unit of energy each pass
void copter_wall(struct Player * p, bool touching);

// The copters crash into each other
void copters_collide(char sprite_collisions);

#pragma compile("copter.c")

#endif
