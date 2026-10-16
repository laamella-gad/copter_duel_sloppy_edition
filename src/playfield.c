#include "playfield.h"
#include "system.h"
#include <c64/vic.h>

static const char playfield_chars[1000] = {
	#embed "playfield_chars.bin"
};
static const char playfield_colours[1000] = {
	#embed "playfield_colours.bin"
};

static char odd_pass;
static char bonus_delay, bonuses_placed;
static char bonus_offsets[4];

static const char bonus_chars[4] = {BONUS_POINTS, BONUS_ROCKETS, BONUS_POINTS, BONUS_ENERGY};

// The screen quarters a bonus is placed in, at an 8 bit offset each
static volatile char * const bonus_areas[4] = {screen, screen + 0x0d8, screen + 0x1d2, screen + 0x2cc};

void playfield_draw(void)
{
	vic.ctrl1 = 0;
	for (unsigned i = 0; i < 1000; i++)
	{
		screen[i] = playfield_chars[i];
		colour_ram[i] = playfield_colours[i];
	}
	vic.ctrl1 = 27;
}

bool is_bonus(char c)
{
	return c == BONUS_POINTS || c == BONUS_ROCKETS || c == BONUS_ENERGY;
}

void bonuses_reset(void)
{
	odd_pass = 0;
	bonus_delay = 0;
	bonuses_placed = 0;
}

void bonuses_update(void)
{
	odd_pass ^= 1;
	if (!odd_pass)
		return;
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

	// The raster line as a random number
	for (char i = 0; i < 4; i++)
	{
		do {
			bonus_offsets[i] ^= vic.raster;
		} while (bonus_areas[i][bonus_offsets[i]] != BLANK);
		bonus_areas[i][bonus_offsets[i]] = bonus_chars[vic.raster & 3];
	}
}
