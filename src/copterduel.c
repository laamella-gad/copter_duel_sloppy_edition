// Copter Duel, (c) 1987 Ronald Mayer: the game rewritten in C for oscar64.
// This file is the original's BASIC program (the comments give its line
// numbers); engine.c is its machine code round, window.c its window routines.
#include <c64/vic.h>
#include <c64/cia.h>
#include "engine.h"
#include "window.h"

// Code and variables stay out of VIC bank 1 ($4000-$7fff) where the
// screen ($4000), the sprites ($4400) and the character set ($7800) are.
#pragma region(main, 0x0a00, 0x4000, , , {code, data, bss, heap, stack})
#pragma stacksize(1024)
#pragma heapsize(0)

#pragma section(sprites, 0)
#pragma region(sprites, 0x4400, 0x4cc0, , , {sprites})
#pragma section(charset, 0)
#pragma region(charset, 0x7800, 0x8000, , , {charset})

#pragma data(sprites)
__export const char sprite_data[] = {
	#embed "sprites.bin"
};
#pragma data(charset)
__export const char charset_data[] = {
	#embed "charset.bin"
};
#pragma data(data)

#define POKE(a, v)  (*(volatile char *)(a) = (v))
#define PEEK(a)     (*(volatile char *)(a))

// KERNAL variables
#define KEY_PRESSED     203     // 64: no key
#define SHIFT_MODE      657     // 128: Shift + C= doesn't switch the character set
#define JIFFY_LO        0xa2

#define sid_reg ((volatile char *)0xd400)

// Colours
#define WHITE       1
#define RED         2
#define YELLOW      7
#define GREEN       5
#define LIGHT_BLUE  14

// The original printed through the KERNAL; like its screen editor, a clear
// fills the colour RAM with the colour of the last text
static char text_colour = LIGHT_BLUE;

static void clear_screen(volatile char * at)
{
	for (unsigned i = 0; i < 1000; i++)
	{
		at[i] = ' ';
		colour_ram[i] = text_colour;
	}
}

// Writes screen code text at a row and column
static void put(volatile char * at, char row, char column, char colour, const char * s)
{
	unsigned o = row * 40 + column;
	text_colour = colour;
	while (*s)
	{
		at[o] = *s++;
		colour_ram[o] = colour;
		o++;
	}
}

// Waits the time of a BASIC FOR loop: forj=1to2000:next is 124 jiffies
static void wait_jiffies(char n)
{
	char t = PEEK(JIFFY_LO);
	while ((char)(PEEK(JIFFY_LO) - t) < n)
		;
}

static bool no_key_or_button(void)
{
	return PEEK(KEY_PRESSED) == 64 && cia1.pra == 127 && cia1.prb == 255;
}

// 1079-1109: the playfield and the status panels, yellow top, red bottom
static const char playfield_chars[1000] = {
	#embed "playfield_chars.bin"
};
static const char playfield_colours[1000] = {
	#embed "playfield_colours.bin"
};

static void draw_screen(void)
{
	vic.ctrl1 = 0;
	for (unsigned i = 0; i < 1000; i++)
	{
		screen[i] = playfield_chars[i];
		colour_ram[i] = playfield_colours[i];
	}
	vic.ctrl1 = 27;
}

static void sid_setup(void)
{
	for (char i = 0; i <= 24; i++)
		sid_reg[i] = 0;
	sid_reg[6] = 240;
	sid_reg[4] = 64;
	sid_reg[4] = 65;
	sid_reg[2] = 1;
	sid_reg[3] = 1;
	sid_reg[24] = 15;
	sid_reg[13] = 250;
	sid_reg[11] = 128;
	sid_reg[8] = 10;
}

static const char fade[7] = {11, 12, 15, 1, 15, 12, 11};

// 1017-1048: "mr-soft presents" fading in and out (the original showed it
// before every title, this version only at the start)
static void mr_soft_presents(void)
{
	for (char i = 0; i <= 3; i++)
	{
		put(screen, 11, 11, fade[i], s"mr-soft  presents");
		wait_jiffies(7);
	}
	wait_jiffies(124);
	for (char i = 4; i <= 6; i++)
	{
		put(screen, 11, 11, fade[i], s"mr-soft  presents");
		wait_jiffies(7);
	}
}

// 1017-1048: the title, until a key or button is pressed
static void title(void)
{
	clear_screen(screen);
	vic.spr_enable = 0;
	vic.ctrl2 = 200;
	vic.color_back = 0;
	vic.color_border = 0;
	vic.spr_multi = 255;
	vic.spr_mcolor0 = 7;
	vic.spr_mcolor1 = 9;
	for (char i = 0; i <= 5; i++)
		vic.spr_color[i] = 10;
	vic.spr_color[6] = 7;
	vic.spr_color[7] = 7;
	sid_setup();

	for (char i = 0; i <= 5; i++)
	{
		vic.spr_pos[i].y = 80;
		vic.spr_pos[i].x = 110 + i * 25;
	}
	for (char i = 0; i <= 7; i++)
		sprite_pointers[i] = 43 + i;
	vic.spr_pos[6].x = 160;
	vic.spr_pos[7].x = 184;
	vic.spr_pos[6].y = 110;
	vic.spr_pos[7].y = 110;
	vic.spr_enable = 255;
	// The @ (the copyright sign in this character set) is screen code 0
	put(screen, 13, 10, LIGHT_BLUE, s" 1987 by ronald mayer");
	screen[13 * 40 + 9] = 0;
	colour_ram[13 * 40 + 9] = LIGHT_BLUE;
	put(screen, 18, 4, GREEN, s"press any key or button to start");

	while (no_key_or_button())
		;
}

// 0 if equal, else which 5 digit score is higher: 2 yellow (p1), 1 red (p2)
static char compare_scores(void)
{
	for (char i = 0; i < 5; i++)
	{
		if (yellow_score[i] > red_score[i])
			return 2;
		if (red_score[i] > yellow_score[i])
			return 1;
	}
	return 0;
}

// 1049-1078: one game, then who won
static void game(void)
{
	vic.spr_enable = 0;
	clear_screen(screen);
	vic.color_border = 8;
	vic.color_back = 14;
	vic.color_back1 = 11;
	vic.color_back2 = 8;
	vic.ctrl2 = 216;
	draw_screen();

	vic.spr_multi = 240;
	sprite_pointers[0] = 22;
	sprite_pointers[1] = 17;
	sprite_pointers[2] = 32;
	sprite_pointers[3] = 32;
	sprite_pointers[4] = 33;
	sprite_pointers[5] = 33;
	vic.spr_color[0] = 7;
	vic.spr_color[1] = 2;
	vic.spr_color[2] = 0;
	vic.spr_color[3] = 0;
	vic.spr_color[4] = 8;
	vic.spr_color[5] = 8;
	vic.spr_color[6] = 3;
	vic.spr_color[7] = 13;
	vic.spr_priority = 255;
	vic.spr_sprcol = 0;
	vic.spr_backcol = 0;
	vic.spr_pos[0].x = 40;
	vic.spr_pos[1].x = 230;
	vic.spr_pos[0].y = 225;
	vic.spr_pos[1].y = 225;
	vic.spr_enable = 3;

	sid_reg[0] = 190;
	play_round();
	sid_reg[0] = 0;

	char x = game_over_flags;

	window_open(screen + 242, 26, 10, 3);
	window_open(screen + 283, 24, 8, 6);
	put(screen, 9, 11, RED, s"game over");

	if (x != 1 && x != 2)
		x = compare_scores();
	if (x == 2)
		put(screen, 13, 6, YELLOW, s"yellow copter wins!");
	else if (x == 1)
		put(screen, 13, 8, RED, s"red copter wins!");
	else
	{
		put(screen, 12, 7, WHITE, s"both players have");
		put(screen, 14, 9, WHITE, s"equal points!");
	}

	vic.spr_enable = 0;
	wait_jiffies(125);
	while (no_key_or_button())
		;
	window_close();
	window_close();
	window_close();
}

int main(void)
{
	vic.color_back = 0;
	vic.color_border = 0;

	// 1014-1016: no case switching, VIC bank 1 with the screen at $4000
	// and the character set at $7800
	POKE(SHIFT_MODE, 128);
	cia2.pra = 2;
	vic.memptr = 15;
	clear_screen(screen);
	vic.spr_enable = 0;
	vic.spr_msbx = 0;
	vic.spr_expand_y = 0;
	vic.spr_expand_x = 0;
	vic.color_back1 = 7;
	vic.color_back2 = 9;

	mr_soft_presents();
	for (;;)
	{
		title();
		game();
	}
	return 0;
}
