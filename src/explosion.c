#include "explosion.h"
#include "system.h"
#include "sound.h"
#include "playfield.h"
#include <c64/vic.h>

// The background's colours in the frames after an explosion, last first
static const char flash_colours[] = {
	PLAYFIELD_BACK, BLACK, BLACK, WHITE, WHITE, BLACK, BLACK
};

static char frame_delay;
static volatile char flash;

void explosion_reset(void)
{
	vic.spr_enable &= ~(1 << EXPLOSION_SPRITE);
	frame_delay = 0;
	flash = 0;
	vic.color_back = PLAYFIELD_BACK;
}

__interrupt void explosion_flash(void)
{
	if (flash)
		vic.color_back = flash_colours[--flash];
}

void explode(char x, char y)
{
	vic.spr_pos[EXPLOSION_SPRITE].x = x;
	vic.spr_pos[EXPLOSION_SPRITE].y = y;
	sprite_pointers[EXPLOSION_SPRITE] = EXPLOSION_FIRST;
	vic.spr_enable |= 1 << EXPLOSION_SPRITE;
	frame_delay = EXPLOSION_FRAME - 1;
	flash = sizeof(flash_colours);
	sound_explosion();
}

void explosion_update(void)
{
	if (!(vic.spr_enable & (1 << EXPLOSION_SPRITE)))
		return;
	if (frame_delay)
	{
		frame_delay--;
		return;
	}
	frame_delay = EXPLOSION_FRAME - 1;
	if (sprite_pointers[EXPLOSION_SPRITE] < EXPLOSION_LAST)
		sprite_pointers[EXPLOSION_SPRITE]++;
	else
	{
		vic.spr_enable &= ~(1 << EXPLOSION_SPRITE);
		sound_explosion_end();
	}
}
