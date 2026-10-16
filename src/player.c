#include "player.h"
#include "panel.h"
#include "copter.h"
#include "rocket.h"
#include <c64/vic.h>
#include <c64/cia.h>

struct Player yellow = {
	0, &cia1.pra, screen + 0x0c0, screen + 0x138, screen + 0x1b1, yellow_score, red_score,
	0x28, YELLOW, 0x17, 0x01         // home: x, colour, flying right after a crash
};

struct Player red = {
	1, &cia1.prb, screen + 0x2c8, screen + 0x340, screen + 0x3b9, red_score, yellow_score,
	0xe6, RED, 0x10, 0x02            // home: x, colour, flying left after a crash
};

char game_over_flags;

static void reset(struct Player * p)
{
	p->x = vic.spr_pos[p->sprite].x << 8;
	p->y = vic.spr_pos[p->sprite].y << 8;
	p->dx = p->dy = 0;
	p->turn_delay = 0;
	p->crashed = 0;
	p->crash_delay = 0;
	p->rocket = ROCKET_NONE;
}

void players_reset(void)
{
	reset(&yellow);
	reset(&red);
	game_over_flags = 0;
}

void use_energy(struct Player * p)
{
	if (!bar_take(p->energy_bar))
		copter_crash(p);
}

void player_tick(struct Player * p)
{
	if (p->tick_delay)
	{
		p->tick_delay--;
		return;
	}
	p->tick_delay = 250;
	score_add(p->score, POINTS_1);
	use_energy(p);
}
