#include "rocket.h"
#include "explosion.h"
#include "panel.h"
#include "playfield.h"
#include "sound.h"
#include <c64/vic.h>

// Energy units a hit takes: a third of a full bar
#define DAMAGE          9

void rocket_fire(struct Player * p, char direction)
{
	char s = p->sprite;

	if (p->rocket || !bar_take(p->rocket_bar))
		return;
	vic.spr_pos[s + 2].x = vic.spr_pos[s].x;
	vic.spr_pos[s + 2].y = vic.spr_pos[s].y;
	vic.spr_enable |= 4 << s;
	p->rocket = direction;
	sound_rocket();
}

static void rocket_end(struct Player * p)
{
	vic.spr_enable &= ~(4 << p->sprite);
	p->rocket = ROCKET_NONE;
}

void rocket_move(struct Player * p)
{
	char s = p->sprite + 2;

	if (p->rocket == ROCKET_LEFT)
		vic.spr_pos[s].x -= 2;
	else if (p->rocket == ROCKET_RIGHT)
		vic.spr_pos[s].x += 2;
}

// The screen cell the rocket picks bonuses up from: one row down and two
// columns right of its top left corner; none off the screen
static volatile char * rocket_cell(struct Player * p)
{
	char s = p->sprite + 2;
	char column = (char)(vic.spr_pos[s].x - 0x1a) >> 3;
	char row = (char)(vic.spr_pos[s].y - 0x30) >> 3;
	if (row >= 24 || column >= 38)
		return nullptr;
	return screen + (unsigned)row * 40 + column + 0x2a;
}

void rocket_wall(struct Player * p, bool touching)
{
	if (!p->rocket || !touching)
		return;
	volatile char * cell = rocket_cell(p);
	if (cell && is_bonus(*cell))
		return;

	char s = p->sprite + 2;
	rocket_end(p);
	explode(vic.spr_pos[s].x, vic.spr_pos[s].y);
}

void rocket_hit(struct Player * p, struct Player * target, char sprite_collisions)
{
	char c = target->sprite, r = p->sprite + 2;
	char both = (1 << c) | (1 << r);

	if ((sprite_collisions & both) != both || !p->rocket || target->crashed)
		return;

	char t = vic.spr_pos[c].x - 0x10;
	if (t < vic.spr_pos[r].x && (char)(vic.spr_pos[r].x - t) < 0x20)
	{
		t = vic.spr_pos[c].y - 0x06;
		if (t < vic.spr_pos[r].y && (char)(vic.spr_pos[r].y - t) < 0x0c)
		{
			rocket_end(p);
			for (char i = 0; i < DAMAGE && !target->crashed; i++)
				use_energy(target);
		}
	}
}

void rocket_bonus(struct Player * p)
{
	if (!p->rocket)
		return;

	volatile char * cell = rocket_cell(p);
	if (!cell)
		return;

	switch (*cell)
	{
	case BONUS_POINTS:
		*cell = BLANK;
		score_add(p->score, POINTS_10);
		break;
	case BONUS_ROCKETS:
		*cell = BLANK;
		bar_refill(p->rocket_bar, 14);
		break;
	case BONUS_ENERGY:
		*cell = BLANK;
		bar_refill(p->energy_bar, 10);
		break;
	}
}
