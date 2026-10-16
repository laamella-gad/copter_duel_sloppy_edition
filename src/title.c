#include "title.h"
#include "system.h"
#include "sound.h"
#include "music.h"
#include <c64/vic.h>

// Sprite blocks $2b-$32: the title's letters
#define TITLE_SPRITES   0x2b

// "copter" (sprites 0-5) waves up and down around y = 80: each letter is
// WAVE_SPREAD entries further on in the sine table, which moves WAVE_SPEED
// entries a frame
#define WAVE_TOP        72
#define WAVE_SPREAD     32
#define WAVE_SPEED      4

// The title letters' wave: 8 + 8 * sin(2 * pi * i / 256), rounded
static const char sine[256] = {
	 8,  8,  8,  9,  9,  9,  9,  9, 10, 10, 10, 10, 10, 11, 11, 11,
	11, 11, 11, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 14,
	14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 14, 14, 14, 14, 14,
	14, 14, 13, 13, 13, 13, 13, 13, 12, 12, 12, 12, 12, 12, 11, 11,
	11, 11, 11, 11, 10, 10, 10, 10, 10,  9,  9,  9,  9,  9,  8,  8,
	 8,  8,  8,  7,  7,  7,  7,  7,  6,  6,  6,  6,  6,  5,  5,  5,
	 5,  5,  5,  4,  4,  4,  4,  4,  4,  3,  3,  3,  3,  3,  3,  2,
	 2,  2,  2,  2,  2,  2,  2,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	 1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,
	 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,
	 2,  2,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  5,  5,
	 5,  5,  5,  5,  6,  6,  6,  6,  6,  7,  7,  7,  7,  7,  8,  8
};

static const char fade[7] = {11, 12, 15, 1, 15, 12, 11};

void mr_soft_presents(void)
{
	for (char i = 0; i <= 3; i++)
	{
		put_text(11, 11, fade[i], s"mr-soft  presents");
		wait_jiffies(7);
	}
	wait_jiffies(124);
	for (char i = 4; i <= 6; i++)
	{
		put_text(11, 11, fade[i], s"mr-soft  presents");
		wait_jiffies(7);
	}
}

void title(void)
{
	clear_screen();
	vic.spr_enable = 0;
	vic.ctrl2 = 200;
	vic.color_back = BLACK;
	vic.color_border = BLACK;
	music_start();

	// The title in sprites: six letters in a row, two below
	vic.spr_multi = 255;
	vic.spr_mcolor0 = 7;
	vic.spr_mcolor1 = 9;
	for (char i = 0; i <= 5; i++)
	{
		vic.spr_color[i] = 10;
		vic.spr_pos[i].y = 80;
		vic.spr_pos[i].x = 110 + i * 25;
	}
	vic.spr_color[6] = 7;
	vic.spr_color[7] = 7;
	for (char i = 0; i <= 7; i++)
		sprite_pointers[i] = TITLE_SPRITES + i;
	vic.spr_pos[6].x = 160;
	vic.spr_pos[7].x = 184;
	vic.spr_pos[6].y = 110;
	vic.spr_pos[7].y = 110;
	vic.spr_enable = 255;

	// The @ (the copyright sign in this character set) is screen code 0
	put_text(13, 10, LIGHT_BLUE, s" 1987 by ronald mayer");
	screen[13 * 40 + 9] = 0;
	colour_ram[13 * 40 + 9] = LIGHT_BLUE;
	put_text(15, 3, GREY, s"slopped by claude and laamella gad");
	put_text(18, 4, GREEN, s"press any key or button to start");

	char wave = 0;
	while (!key_or_button())
	{
		vic_waitFrame();
		for (char i = 0; i <= 5; i++)
			vic.spr_pos[i].y = WAVE_TOP + sine[(char)(wave + i * WAVE_SPREAD)];
		wave += WAVE_SPEED;
	}
	music_stop();
	sound_setup();
}
