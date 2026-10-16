// The two players. Yellow flies sprite 0 with the joystick in port 2, red
// sprite 1 with the joystick in port 1; each has a rocket (sprite + 2) and
// a status panel.
#ifndef PLAYER_H
#define PLAYER_H

struct Player
{
	char                sprite;         // copter sprite; the rocket is sprite + 2
	volatile char     * joystick;
	volatile char     * energy_bar;
	volatile char     * rocket_bar;
	volatile char     * copter_icons;
	volatile char     * score;
	volatile char     * other_score;
	char                home_x, colour, home_pointer;
	char                lost_flag;

	// The copter
	unsigned            x, y;           // position, 8.8 fixed point
	int                 dx, dy;         // speed in pixels per pass, 8.8 fixed point
	char                turn_delay;     // passes until the next turn step
	char                crashed;        // exploding, until it comes back or the player is out
	char                crash_delay;
	char                crash_step;

	char                rocket;         // ROCKET_NONE, ROCKET_RIGHT or ROCKET_LEFT
	char                tick_delay;
};

extern struct Player yellow, red;

// Bit 0: yellow has no copters left, bit 1: red
extern char game_over_flags;

// The start of a round: the copters where their sprites are, standing still
void players_reset(void);

// Takes a unit of energy; out of energy the copter crashes
void use_energy(struct Player * p);

// Each pass: every 251 passes a point and a unit of energy less
void player_tick(struct Player * p);

#pragma compile("player.c")

#endif
