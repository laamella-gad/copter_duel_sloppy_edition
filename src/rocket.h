// The rockets: they fly sideways until they hit the background or the
// other copter, and pick up the bonuses they fly over
#ifndef ROCKET_H
#define ROCKET_H

#include "player.h"

#define ROCKET_NONE     0
#define ROCKET_RIGHT    1
#define ROCKET_LEFT     2

// Fires the rocket, unless it is flying or the rocket bar is empty
void rocket_fire(struct Player * p, char direction);

// Each pass: 2 pixels further
void rocket_move(struct Player * p);

// On the background the rocket explodes, unless it is over a bonus
void rocket_wall(struct Player * p, bool touching);

// The rocket hits the other copter: it is gone and the copter loses a third
// of its energy
void rocket_hit(struct Player * p, struct Player * target, char sprite_collisions);

// Each pass: picks up the bonus the rocket flies over
void rocket_bonus(struct Player * p);

#pragma compile("rocket.c")

#endif
