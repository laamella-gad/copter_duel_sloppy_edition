#include "title.h"
#include "system.h"
#include "sound.h"
#include "music.h"
#include <c64/vic.h>

// Sprite blocks $2b-$32: the title's letters
#define TITLE_SPRITES   0x2b

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
	put_text(18, 4, GREEN, s"press any key or button to start");

	while (!key_or_button())
		;
	music_stop();
	sound_setup();
}
