#include "copter.h"
#include "rocket.h"
#include "explosion.h"
#include "panel.h"
#include "sound.h"
#include <c64/vic.h>

// Sprite blocks $10-$1f: 8 headings, each in two rotor frames (bit 3).
// With the rotor bit set: $18 flying left, $19 level left, $1a-$1d turning
// through the front, $1e level right, $1f flying right.
#define ROTOR_FRAME     0x08
#define FLY_LEFT        0x18
#define LEVEL_LEFT      0x19
#define LEVEL_RIGHT     0x1e
#define FLY_RIGHT       0x1f

#define GROUND_Y        0xe8
#define HOME_Y          0xe4

// A crashed copter explodes for EXPLOSION_STEPS explosion frames, spinning
// through all its sprite blocks one per pass and through the colours one per
// frame, then is hidden; it comes back
// 10 frames later
#define EXPLOSION_STEPS (EXPLOSION_LAST - EXPLOSION_FIRST)
#define CRASH_STEPS     (EXPLOSION_STEPS + 10)
#define COPTER_FIRST    0x10
#define COPTER_FRAMES   16

// Positions and speeds are 8.8 fixed point (the high byte is the sprite's
// pixel position). Each pass the joystick adds THRUST to the speed and drag
// takes 1/2^DRAG_SHIFT of it away, so the top speed is THRUST << DRAG_SHIFT:
// 1 pixel per pass, the original's only speed.
#define THRUST          16
#define DRAG_SHIFT      4

// The raster IRQ's line: below the playfield, so colours change unseen
#define EFFECTS_LINE    251

char irq_odd;
static char gravity_delay;

// Each frame, from the raster IRQ: exploding copters flash through the colours
static __interrupt void copters_flash(void)
{
	if (yellow.crashed && yellow.crash_step < EXPLOSION_STEPS)
		vic.spr_color[yellow.sprite]++;
	if (red.crashed && red.crash_step < EXPLOSION_STEPS)
		vic.spr_color[red.sprite]++;
}

// The raster IRQ: the colour effects. The KERNAL's CIA IRQ: every other one
// toggles the copters between rotor frames, then the KERNAL IRQ.
__asm round_irq
{
	lda $d019
	and #$01
	beq rotor_irq
	sta $d019
	jsr copters_flash
	jsr explosion_flash
	jmp $ea81
rotor_irq:
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

void copters_start(void)
{
	__asm { sei }
	*(void **)0x0314 = round_irq;
	vic.raster = EFFECTS_LINE;
	vic.ctrl1 &= 0x7f;
	vic.intr_ctrl = 0xff;
	vic.intr_enable = 1;
	__asm { cli }
	irq_odd = 0;
	gravity_delay = 0;
}

void copters_stop(void)
{
	vic.intr_enable = 0;
	vic.intr_ctrl = 0xff;
}

// Moves the copter's sprite to its position
static void place(struct Player * p)
{
	vic.spr_pos[p->sprite].x = p->x >> 8;
	vic.spr_pos[p->sprite].y = p->y >> 8;
}

static void sink(struct Player * p)
{
	if (p->y < (unsigned)GROUND_Y << 8)
	{
		p->y += 0x100;
		place(p);
	}
}

void copters_gravity(void)
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

// Takes 1/2^DRAG_SHIFT of the speed away, at least 1/256 pixel so it ends at 0
static int drag(int v)
{
	int d = v >> DRAG_SHIFT;
	if (!d && v > 0)
		d = 1;
	return v - d;
}

// The heading with the rotor bit set
static char heading(struct Player * p)
{
	return sprite_pointers[p->sprite] | ROTOR_FRAME;
}

void copter_control(struct Player * p)
{
	char s = p->sprite;
	char joystick = *p->joystick;

	if (p->crashed)
	{
		copter_crash(p);
		return;
	}

	// Neither left nor right: level out (in one rotor frame)
	if ((joystick & (JOY_LEFT | JOY_RIGHT)) == (JOY_LEFT | JOY_RIGHT))
	{
		char f = sprite_pointers[s];
		if (f == FLY_LEFT)
			sprite_pointers[s]++;
		else if (f == FLY_RIGHT)
			sprite_pointers[s]--;
	}

	if (!(joystick & JOY_UP))
		p->dy -= THRUST;
	if (!(joystick & JOY_DOWN))
		p->dy += THRUST;

	// Left: turn towards the left, then fly left; right the other way
	if (!(joystick & JOY_LEFT))
	{
		if (heading(p) == FLY_LEFT)
			p->dx -= THRUST;
		else if (p->turn_delay)
			p->turn_delay--;
		else
		{
			p->turn_delay = 4;
			sprite_pointers[s]--;
		}
	}
	else if (!(joystick & JOY_RIGHT))
	{
		if (heading(p) == FLY_RIGHT)
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

	// Fire sideways, not while facing the front
	if (!(joystick & JOY_FIRE))
	{
		char h = heading(p);
		if (h <= LEVEL_LEFT)
			rocket_fire(p, ROCKET_LEFT);
		else if (h >= LEVEL_RIGHT)
			rocket_fire(p, ROCKET_RIGHT);
	}
}

void copter_crash(struct Player * p)
{
	char s = p->sprite;

	if (p->crashed && p->crash_step < EXPLOSION_STEPS)
		sprite_pointers[s] = COPTER_FIRST + (sprite_pointers[s] + 1) % COPTER_FRAMES;

	if (p->crashed && p->crash_delay)
	{
		p->crash_delay--;
		return;
	}
	p->crash_delay = EXPLOSION_FRAME - 1;

	if (!p->crashed)
	{
		p->crashed = 1;
		explode(vic.spr_pos[s].x, vic.spr_pos[s].y);
		p->crash_step = 0;
		p->dx = p->dy = 0;
		score_add(p->other_score, POINTS_100);
		return;
	}

	if (++p->crash_step == EXPLOSION_STEPS)
	{
		vic.spr_enable &= ~(1 << s);
		vic.spr_color[s] = p->colour;
	}
	if (p->crash_step < CRASH_STEPS)
		return;

	p->crashed = 0;
	if (!lose_copter(p->copter_icons))
	{
		game_over_flags |= p->lost_flag;
		return;
	}

	bar_fill(p->energy_bar);
	bar_fill(p->rocket_bar);
	p->x = p->home_x << 8;
	p->y = HOME_Y << 8;
	place(p);
	sprite_pointers[s] = p->home_pointer;
	vic.spr_enable |= 1 << s;
	vic.spr_backcol = 0;
}

void copter_wall(struct Player * p, bool touching)
{
	if (touching && !p->crashed)
	{
		sound_scrape();
		use_energy(p);
	}
}

void copters_collide(char sprite_collisions)
{
	if ((sprite_collisions & 0x03) != 0x03 || yellow.crashed || red.crashed)
		return;

	char t = vic.spr_pos[0].x - 0x18;
	if (t < vic.spr_pos[1].x && (char)(vic.spr_pos[1].x - t) < 0x30)
	{
		t = vic.spr_pos[0].y - 0x0b;
		if (t < vic.spr_pos[1].y && (char)(vic.spr_pos[1].y - t) < 0x16)
		{
			copter_crash(&yellow);
			copter_crash(&red);
		}
	}
}
