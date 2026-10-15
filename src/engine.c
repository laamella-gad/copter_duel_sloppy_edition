// The round: a port of the machine code engine at $8000 (sys32768 in the original).
// Yellow is sprite 0 with its rocket in sprite 2, joystick port 2;
// red is sprite 1 with its rocket in sprite 3, joystick port 1.
#include "engine.h"
#include <c64/vic.h>
#include <c64/cia.h>
#include <c64/sid.h>

// One pass of the original round_loop took 13136 cycles on average,
// measured in VICE; CIA 2 timer A paces the loop at that rate.
#define LOOP_CYCLES     13136

// Status bar characters
#define BAR_FULL        0x23
#define BAR_EMPTY       0x27
#define ICON_LOST       0x3d
#define BLANK           0x20

// Bonuses: 10 points, 3 rockets, 5 energy
#define BONUS_POINTS    0x3a
#define BONUS_ROCKETS   0x3b
#define BONUS_ENERGY    0x3c

// Sprite blocks: copter headings $10-$1f (bit 3 is the rotor frame),
// level flight right $18 and left $1f, explosion $21-$2a
#define COPTER_RIGHT    0x18
#define COPTER_LEFT     0x1f
#define EXPLOSION_FIRST 0x21
#define EXPLOSION_LAST  0x2a

#define GROUND_Y        0xe8

// Copter physics: positions and speeds are 8.8 fixed point (the high byte is
// the sprite's pixel position). Each pass the joystick adds THRUST to the
// speed and drag takes 1/2^DRAG_SHIFT of it away, so the top speed is
// THRUST << DRAG_SHIFT: 1 pixel per pass, the original's only speed.
#define THRUST          16
#define DRAG_SHIFT      4

struct Player
{
	char                sprite;         // copter sprite; the rocket is sprite + 2
	volatile char     * joystick;
	volatile char     * energy_bar;     // 7 characters each
	volatile char     * rocket_bar;
	volatile char     * copter_icons;   // icons at 0, 2, 4
	volatile char     * score;
	volatile char     * other_score;
	char                home_x, colour, home_pointer;
	char                lost_flag;

	unsigned            x, y;           // position, 8.8 fixed point
	int                 dx, dy;         // speed in pixels per pass, 8.8 fixed point

	char                turn_delay;     // frames until the next turn step
	char                rocket;         // 0 none, 1 flying right, 2 flying left
	char                wall_frames;    // frames in a row touching the background
	char                exploding;
	char                explosion_delay;
	char                explosion_step;
	char                tick_delay;
};

static struct Player yellow = {
	0, &cia1.pra, screen + 0x0c0, screen + 0x138, screen + 0x1b1, yellow_score, red_score,
	0x28, 7, 0x17, 0x01
};

static struct Player red = {
	1, &cia1.prb, screen + 0x2c8, screen + 0x340, screen + 0x3b9, red_score, yellow_score,
	0xe6, 2, 0x10, 0x02
};

char game_over_flags;

char irq_odd;
static char sprite_collisions, background_collisions;
static char gravity_delay;
static char odd_frame;
static char bonus_delay, bonuses_placed;
static char bonus_offsets[4];

static const char bonus_chars[4] = {BONUS_POINTS, BONUS_ROCKETS, BONUS_POINTS, BONUS_ENERGY};

// The screen quarters a bonus is placed in, at an 8 bit offset each
static volatile char * const bonus_areas[4] = {screen, screen + 0x0d8, screen + 0x1d2, screen + 0x2cc};

// Start addresses of the screen rows, low bytes then high bytes
static const char screen_rows[25 + 25] = {
	0x00, 0x28, 0x50, 0x78, 0xa0, 0xc8, 0xf0, 0x18, 0x40, 0x68, 0x90, 0xb8, 0xe0, 0x08, 0x30, 0x58,
	0x80, 0xa8, 0xd0, 0xf8, 0x20, 0x48, 0x70, 0x98, 0xc0,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x42, 0x42, 0x42,
	0x42, 0x42, 0x42, 0x42, 0x43, 0x43, 0x43, 0x43, 0x43
};

// Every other IRQ: toggles the copters between rotor frames, then the KERNAL IRQ
__asm irq_rotor_animation
{
	lda irq_odd
	eor #$01
	sta irq_odd
	and #$01
	beq rotor_done
	lda $43f8
	cmp #$20
	bcs rotor_red
	eor #$08
	sta $43f8
rotor_red:
	lda $43f9
	cmp #$20
	bcs rotor_done
	eor #$08
	sta $43f9
rotor_done:
	jmp $ea31
}

// Moves the copter's sprite to its position
static void place(struct Player * p)
{
	vic.spr_pos[p->sprite].x = p->x >> 8;
	vic.spr_pos[p->sprite].y = p->y >> 8;
}

// Takes 1/2^DRAG_SHIFT of the speed away, at least 1/256 pixel so it ends at 0
static int drag(int v)
{
	int d = v >> DRAG_SHIFT;
	if (!d && v > 0)
		d = 1;
	return v - d;
}

// Adds 1 to digit x of a score (x = 4: 1 point, 3: 10, 2: 100)
static void score_add(volatile char * score, signed char x)
{
	for (;;)
	{
		if (score[x] < '9')
		{
			score[x]++;
			return;
		}
		score[x] = '0';
		if (--x < 0)
			return;
	}
}

// Starts or steps the explosion; after it a copter is lost and the copter
// is reset, or, without copters left, the player is out
static void hit(struct Player * p)
{
	char s = p->sprite;

	if (p->exploding && p->explosion_delay)
	{
		p->explosion_delay--;
		return;
	}
	p->explosion_delay = 8;

	if (!p->exploding)
	{
		p->exploding++;
		vic.spr_multi |= 1 << s;
		sprite_pointers[s] = EXPLOSION_FIRST;
		sid.voices[1].ctrl = SID_CTRL_NOISE | SID_CTRL_GATE;
		sid.voices[1].ctrl = SID_CTRL_NOISE;
		vic.spr_color[s] = 8;
		p->explosion_step = 0;
		p->dx = p->dy = 0;
		score_add(p->other_score, 2);
		return;
	}

	if (sprite_pointers[s] < EXPLOSION_LAST)
	{
		sprite_pointers[s]++;
		return;
	}
	if (++p->explosion_step < 10)
		return;

	p->exploding--;
	signed char x = 4;
	while (p->copter_icons[x] == ICON_LOST)
	{
		x -= 2;
		if (x < 0)
		{
			vic.spr_enable &= ~(1 << s);
			game_over_flags |= p->lost_flag;
			return;
		}
	}
	p->copter_icons[x] = ICON_LOST;

	for (signed char i = 6; i >= 0; i--)
	{
		p->energy_bar[i] = BAR_FULL;
		p->rocket_bar[i] = BAR_FULL;
	}
	vic.spr_multi &= ~(1 << s);
	p->x = p->home_x << 8;
	p->y = 0xe4 << 8;
	place(p);
	vic.spr_color[s] = p->colour;
	sprite_pointers[s] = p->home_pointer;
	vic.spr_backcol = 0;
}

static void sink(struct Player * p)
{
	if (p->y < (unsigned)GROUND_Y << 8)
	{
		p->y += 0x100;
		place(p);
	}
}

// Every 30 frames both copters sink one pixel
static void gravity(void)
{
	if (gravity_delay)
	{
		gravity_delay--;
		return;
	}
	gravity_delay = 30;
	sink(&yellow);
	sink(&red);
}

// The joystick: fly, turn, fire. While exploding: the explosion
static void control(struct Player * p)
{
	char s = p->sprite;

	if (p->exploding)
	{
		hit(p);
		return;
	}

	// Neither left nor right: level out
	if ((*p->joystick & 0x0c) == 0x0c)
	{
		char f = sprite_pointers[s];
		if (f == COPTER_RIGHT)
			sprite_pointers[s]++;
		else if (f == COPTER_LEFT)
			sprite_pointers[s]--;
	}

	if (!(*p->joystick & 0x01))
		p->dy -= THRUST;
	if (!(*p->joystick & 0x02))
		p->dy += THRUST;

	// Left: turn towards the left, then fly left; right the other way
	if (!(*p->joystick & 0x04))
	{
		if ((sprite_pointers[s] | 0x08) < 0x19)
			p->dx -= THRUST;
		else if (p->turn_delay)
			p->turn_delay--;
		else
		{
			p->turn_delay = 4;
			sprite_pointers[s]--;
		}
	}
	else if (!(*p->joystick & 0x08))
	{
		if ((sprite_pointers[s] | 0x08) >= COPTER_LEFT)
			p->dx += THRUST;
		else if (p->turn_delay)
			p->turn_delay--;
		else
		{
			p->turn_delay = 4;
			sprite_pointers[s]++;
		}
	}

	p->dx = drag(p->dx);
	p->dy = drag(p->dy);
	p->x += p->dx;
	p->y += p->dy;
	if (p->y >= (unsigned)GROUND_Y << 8)
	{
		p->y = (unsigned)GROUND_Y << 8;
		if (p->dy > 0)
			p->dy = 0;
	}
	place(p);

	// Fire, unless a rocket is flying or the copter faces the screen
	if (p->rocket)
		return;
	char heading = sprite_pointers[s] | 0x08;
	if (heading >= 0x1a && heading < 0x1e)
		return;
	if (*p->joystick & 0x10)
		return;

	signed char x = 6;
	while (p->rocket_bar[x] >= BAR_EMPTY)
	{
		if (--x < 0)
			return;
	}
	p->rocket_bar[x]++;

	vic.spr_pos[s + 2].x = vic.spr_pos[s].x;
	vic.spr_pos[s + 2].y = vic.spr_pos[s].y;
	vic.spr_enable |= 4 << s;
	p->rocket = 1;
	if ((sprite_pointers[s] | 0x08) < 0x1a)
		p->rocket++;
}

// Moves the rocket 2 pixels
static void rocket_move(struct Player * p)
{
	char s = p->sprite + 2;

	if (p->rocket >= 2)
		vic.spr_pos[s].x -= 2;
	else if (p->rocket)
		vic.spr_pos[s].x += 2;
}

// Every 250 frames: 1 point and a unit of energy less; out of energy is a crash
static void tick(struct Player * p)
{
	if (p->tick_delay)
	{
		p->tick_delay--;
		return;
	}
	p->tick_delay = 250;
	score_add(p->score, 4);

	for (signed char x = 6; x >= 0; x--)
	{
		if (p->energy_bar[x] < BAR_EMPTY)
		{
			p->energy_bar[x]++;
			return;
		}
	}
	hit(p);
}

// Rockets end at walls, copters crash on walls (2 frames) and on each other,
// rockets hit copters
static void collisions(void)
{
	if (yellow.rocket && (background_collisions & 0x04))
	{
		vic.spr_enable &= 0xfb;
		yellow.rocket = 0;
	}
	if (red.rocket && (background_collisions & 0x08))
	{
		vic.spr_enable &= 0xf7;
		red.rocket = 0;
	}

	if (!(background_collisions & 0x01))
		yellow.wall_frames = 0;
	else if (++yellow.wall_frames >= 2)
	{
		yellow.wall_frames = 0;
		hit(&yellow);
	}
	if (!(background_collisions & 0x02))
		red.wall_frames = 0;
	else if (++red.wall_frames >= 2)
	{
		red.wall_frames = 0;
		hit(&red);
	}

	char t;

	// The copters collide (neither exploding)
	if ((sprite_collisions & 0x03) == 0x03 &&
		sprite_pointers[0] < 0x20 && sprite_pointers[1] < 0x20)
	{
		t = vic.spr_pos[0].x - 0x18;
		if (t < vic.spr_pos[1].x && (char)(vic.spr_pos[1].x - t) < 0x30)
		{
			t = vic.spr_pos[0].y - 0x0b;
			if (t < vic.spr_pos[1].y && (char)(vic.spr_pos[1].y - t) < 0x16)
			{
				hit(&yellow);
				hit(&red);
			}
		}
	}

	// The red rocket hits yellow
	if ((sprite_collisions & 0x09) == 0x09)
	{
		t = vic.spr_pos[0].x - 0x10;
		if (t < vic.spr_pos[3].x && (char)(vic.spr_pos[3].x - t) < 0x20)
		{
			t = vic.spr_pos[0].y - 0x06;
			if (t < vic.spr_pos[3].y && (char)(vic.spr_pos[3].y - t) < 0x0c)
				hit(&yellow);
		}
	}

	// The yellow rocket hits red
	if ((sprite_collisions & 0x06) == 0x06)
	{
		t = vic.spr_pos[1].x - 0x10;
		if (t < vic.spr_pos[2].x && (char)(vic.spr_pos[2].x - t) < 0x20)
		{
			t = vic.spr_pos[1].y - 0x06;
			if (t < vic.spr_pos[2].y && (char)(vic.spr_pos[2].y - t) < 0x0c)
				hit(&red);
		}
	}
}

// Takes n units from the bar, from the left
static void refill(volatile char * bar, char n)
{
	do
	{
		for (char x = 0; x < 7; x++)
		{
			if (bar[x] > BAR_FULL)
			{
				bar[x]--;
				break;
			}
		}
	} while (--n);
}

// The rocket picks up the bonus it flies over
static void rocket_bonus(struct Player * p)
{
	char s = p->sprite + 2;

	if (!p->rocket)
		return;

	char column = (char)(vic.spr_pos[s].x - 0x1a) >> 3;
	char row = (char)(vic.spr_pos[s].y - 0x30) >> 3;
	// The cell is one row down and two columns right; none off the screen
	if (row >= 24 || column >= 38)
		return;
	volatile char * cell = (volatile char *)((screen_rows[25 + row] << 8 | screen_rows[row]) + column + 0x2a);

	switch (*cell)
	{
	case BONUS_POINTS:
		*cell = BLANK;
		score_add(p->score, 3);
		break;
	case BONUS_ROCKETS:
		*cell = BLANK;
		refill(p->rocket_bar, 3);
		break;
	case BONUS_ENERGY:
		*cell = BLANK;
		refill(p->energy_bar, 5);
		break;
	}
}

// Every 200 frames: replaces the four bonuses by new ones at random places
static void place_bonuses(void)
{
	if (bonus_delay)
	{
		bonus_delay--;
		return;
	}
	bonus_delay = 200;

	if (bonuses_placed)
	{
		for (char i = 0; i < 4; i++)
			bonus_areas[i][bonus_offsets[i]] = BLANK;
	}
	bonuses_placed++;

	for (char i = 0; i < 4; i++)
	{
		do {
			bonus_offsets[i] ^= vic.raster;
		} while (bonus_areas[i][bonus_offsets[i]] != BLANK);
		bonus_areas[i][bonus_offsets[i]] = bonus_chars[vic.raster & 3];
	}
}

void play_round(void)
{
	__asm { sei }
	*(void **)0x0314 = irq_rotor_animation;
	__asm { cli }

	irq_odd = 0;
	yellow.turn_delay = red.turn_delay = 0;
	yellow.rocket = red.rocket = 0;
	game_over_flags = 0;
	gravity_delay = 0;
	yellow.exploding = red.exploding = 0;
	yellow.explosion_delay = red.explosion_delay = 0;
	bonus_delay = 0;
	bonuses_placed = 0;
	odd_frame = 0;
	yellow.x = vic.spr_pos[0].x << 8;
	yellow.y = vic.spr_pos[0].y << 8;
	red.x = vic.spr_pos[1].x << 8;
	red.y = vic.spr_pos[1].y << 8;
	yellow.dx = yellow.dy = red.dx = red.dy = 0;

	cia2.ta = LOOP_CYCLES - 1;
	cia2.cra = 0x11;	// start, continuous
	cia2.icr;

	for (;;)
	{
		if (game_over_flags && yellow.exploding + red.exploding == 0)
			break;

		while (!(cia2.icr & 0x01))
			;

		gravity();
		control(&yellow);
		control(&red);
		rocket_move(&yellow);
		rocket_move(&red);
		tick(&yellow);
		tick(&red);
		sprite_collisions = vic.spr_sprcol;
		background_collisions = vic.spr_backcol;
		collisions();
		rocket_bonus(&yellow);
		rocket_bonus(&red);
		odd_frame ^= 1;
		if (odd_frame)
			place_bonuses();
	}

	cia2.cra = 0;
}
